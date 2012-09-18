#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stddef.h>

#include <libedboard.h>
#include "../ledboard/defines.h"

/* create termios.h B<rate> constant from defines.h BAUDRATE */
#define termios_baud_(rate) B ## rate
#define termios_baud(rate) termios_baud_(rate)

#define CLIENT_MAX 10
#define CLIENT_BUFFER_SIZE 2048 /* size for ~1 frame */

/* number of seconds that a client with normal priority has if
 * is more than one client */
#define SCHED_TIME_SEC 10

enum ledloard_client_state {
	IDLE,
	RECEIVING_PRIO,
	RECEIVING_FRAME
};
/* the number of bytes we want to read in each state.
 * we'll use ledloard_client_state as index */
size_t client_state_wants[] = { 1, 1, ARRAY_X_SIZE * ARRAY_Y_SIZE };

struct ledloard_client {
	int fd;
	struct ledloard *loard;
	enum ledboard_priority prio;
	enum ledloard_client_state state;

	uint8_t input_buffer[CLIENT_BUFFER_SIZE];
	size_t write_idx;			/* write position in input_buffer */

	time_t start_time;			/* timestamp when scheduled */
	struct ledloard_client **sched_entry;	/* pointer to entry in scheduler array */
};

struct ledloard {
	int sock_fd;
	int led_fd;
	struct termios term_backup;

	struct pollfd pollfds[CLIENT_MAX + 1]; /* +1 for sock_fd */
	struct ledloard_client clients[CLIENT_MAX];
	size_t nr_clients;
	nfds_t nr_pollfds;

	/* one NULL terminated array of pointers to clients for each priority */
	struct ledloard_client *scheduler[LB_PRIO_GOD - LB_PRIO_NORMAL + 1][CLIENT_MAX + 1];
};

/**
 * ledboard_init() - initialize serial device
 */
int ledboard_init(const char *ledboard, struct termios *term_backup)
{
	int fd;
	struct termios term_new;

	fd = open(ledboard, O_RDWR | O_NOCTTY);
	if (fd == -1)
		return -1;
	/* save old terminal attributes */
	if (tcgetattr(fd, term_backup) == -1)
		return -1;

	/* set baud rates */
	memset(&term_new, 0, sizeof(term_new));
	if (cfsetospeed(&term_new, termios_baud(BAUDRATE)) == -1)
		return -1;
	if (cfsetispeed(&term_new, termios_baud(BAUDRATE)) == -1)
		return -1;
	/* set non-canonical mode */
	term_new.c_lflag &= ~ICANON;
	/* wait for 1 byte to arrive (block) */
	term_new.c_cc[VMIN] = 1;

	if (tcsetattr(fd, TCSANOW, &term_new) == -1)
		return -1;
	return fd;
}

/**
 * ledboard_write() - write raw frame to ledboard
 */
int ledboard_write(int fd, uint8_t frame[ARRAY_Y_SIZE][ARRAY_X_SIZE])
{
	static uint8_t led_buffer[ARRAY_Y_SIZE][ARRAY_X_SIZE * 2 / 8];
	uint8_t ack;
	int i, j, k;

	for (i = 0; i < ARRAY_Y_SIZE; i++) {
		int frame_index = 0;
		for (j = 0; j < ARRAY_X_SIZE * 2 / 8; ++j) {
			uint8_t byte = 0;
			for (k = 0; k < 4; ++k) {
				byte |= (frame[i][frame_index++] & 0x03) << (k * 2);
			}
			led_buffer[i][j] = byte;
		}
	}

	if (write(fd, led_buffer, sizeof(led_buffer)) != sizeof(led_buffer))
		return -1;
	if (read(fd, &ack, sizeof(ack)) != ack)
		return -1;
	if (ack != 0xFE)
		return -1;
	return 0;
}

int sock_nonblock(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * server_init() - initialize server socket
 * @return:     server socket, -1 for error
 */
int server_init(const char *port)
{
	int ret;
	int one = 1;
	int sockfd;
	struct addrinfo hints, *res, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((ret = getaddrinfo(NULL, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	for (p = res; p; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1)
			continue;
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0)
			break;
		close(sockfd);
	}
	freeaddrinfo(res);
	if (p == NULL) {
		fprintf(stderr, "unable to bind\n");
		return -1;
	}

	if (listen(sockfd, 1) == -1) {
		perror("unable to listen");
		return -1;
	}
	if (sock_nonblock(sockfd) == -1) {
		perror("sock_nonblock");
		return -1;
	}
	return sockfd;
}

/**
 * ledloard_init() - open connection to ledloard, setup server socket
 */
int ledloard_init(struct ledloard *loard, const char *ledboard, const char *port)
{
	loard->led_fd = ledboard_init(ledboard, &loard->term_backup);
	if (loard->led_fd == -1)
		return -1;

	loard->sock_fd = server_init(port);
	if (loard->sock_fd == -1)
		return -1;
	loard->pollfds[0].fd = loard->sock_fd;
	loard->pollfds[0].events = POLLIN;
	loard->nr_pollfds = 1;

	loard->nr_clients = 0;
	memset(loard->scheduler, 0, sizeof(loard->scheduler));
	return 0;
}

#define prio_valid(p) ((p) >= LB_PRIO_NORMAL && (p) <= LB_PRIO_GOD)
int prio_idx(enum ledboard_priority prio)
{
	if (prio_valid(prio))
		return prio - LB_PRIO_NORMAL;
	return -1;
}

/**
 * scheduler_client_add() - append pointer to priority array and fill in back pointer
 */
void scheduler_client_add(struct ledloard_client *c)
{
	struct ledloard_client **p;
	for (p = c->loard->scheduler[prio_idx(c->prio)]; *p; p++)
		/* nothing */;
	*p = c;
	c->sched_entry = p;
	c->start_time = 0;
}

/**
 * scheduler_moveup() - move up all clients up, starting (and overwriting) from c
 * @return: pointer to last entry (that is now free)
 */
inline struct ledloard_client **scheduler_moveup(struct ledloard_client **c)
{
	for (; *(c + 1); c++) {
		*c = *(c + 1);
		(*c)->sched_entry = c; /* update back pointer */
	}
	*c = NULL;
	return c;
}

/**
 * scheduler_client_remove() - remove pointer from priority array
 */
void scheduler_client_remove(struct ledloard_client *c)
{
	/* fill the gap by moving the other pointers */
	scheduler_moveup(c->sched_entry);
}

void scheduler_client_update_prio(struct ledloard_client *c)
{
	scheduler_client_remove(c);
	scheduler_client_add(c);
}

/**
 * scheduler_client_update() - update the client pointer
 * @c:   actual client
 * @new: new location of client
 */
void scheduler_client_update(struct ledloard_client *c, struct ledloard_client *new)
{
	struct ledloard_client **entry = c->sched_entry;
	*entry = new;
}

inline bool capo_time_over(struct ledloard_client *c)
{
	return c->start_time + SCHED_TIME_SEC < time(NULL);
}

/**
 * scheduler_capo() - get client that is the boss atm :-)
 */
struct ledloard_client *scheduler_capo(struct ledloard *loard)
{
	struct ledloard_client *capo = NULL;
	int prio;

	for (prio = prio_idx(LB_PRIO_GOD); prio >= 0; prio--) {
		struct ledloard_client **prios = loard->scheduler[prio];
		if (!prios[0])   /* no client in this priority, try next one */
			continue;
		if (prios[1]) {  /* another one with this priority? */
			if (prios[0]->start_time && capo_time_over(prios[0])) {
				struct ledloard_client *oldcapo = prios[0];
				struct ledloard_client **queueback;
				/* put first one (old capo) to the last position
				 * and move the others one up. Then update back pointer
				 * of last one. (The others are updated by scheduler_moveup()
				 */
				queueback = scheduler_moveup(&prios[0]);
				oldcapo->start_time = 0;
				oldcapo->sched_entry = queueback;
				*queueback = oldcapo;
			}
		}
		capo = prios[0];
		if (!capo->start_time)
			capo->start_time = time(NULL);
		return capo;
	}

	return NULL;
}

int ledloard_client_add(struct ledloard *loard, int fd)
{
	struct ledloard_client *c;
	struct pollfd *pfd;

	/* add client to clients array */
	if (loard->nr_clients > CLIENT_MAX)
		return -1;
	c = &loard->clients[loard->nr_clients++];
	c->fd = fd;
	c->loard = loard;
	c->prio = LB_PRIO_NORMAL;
	c->state = IDLE;
	c->write_idx = 0;

	/* add client fd to pollfds */
	pfd = &loard->pollfds[loard->nr_pollfds++];
	pfd->fd = fd;
	pfd->events = POLLIN | POLLPRI;

	scheduler_client_add(c);
	return 0;
}

int client_remove(struct ledloard_client *c)
{
	struct ledloard *loard = c->loard;
	unsigned client_idx;
	int fd = c->fd;

	scheduler_client_remove(c);

	/* if this client is not the last client in our array, we have to fill
	 * the gap by coping the last client to this position in both arrays,
	 * the client array and the corresponding pollfds array */
	client_idx = c - loard->clients;
	if (client_idx != loard->nr_clients - 1) {
		/* update pointer in scheduler because we're moving :) */
		scheduler_client_update(&loard->clients[loard->nr_clients - 1],
					&loard->clients[client_idx]);
		loard->clients[client_idx] = loard->clients[loard->nr_clients - 1];
		loard->pollfds[client_idx + 1] = loard->pollfds[loard->nr_pollfds - 1];
	}
	loard->nr_clients--;
	loard->nr_pollfds--;
	return close(fd);
}

void client_prio_set(struct ledloard_client *c, uint8_t prio)
{
	if (prio_valid(prio)) {
		c->prio = prio;
		scheduler_client_update_prio(c);
	}
}

void ledloard_frame_handle(struct ledloard *loard, struct ledloard_client *c,
					uint8_t frame[ARRAY_Y_SIZE][ARRAY_X_SIZE])
{
	/* check if client is the actual capo */
	if (c == scheduler_capo(loard)) {
		uint8_t ack = 0xF0;
		ledboard_write(loard->led_fd, frame);
		printf("[client %d]: frame written!\n", c->fd);
//		write(c->fd, &ack, sizeof(ack));
	} else {
		printf("[client %d]: gtfo, you're not capo!\n", c->fd);
	}
}

inline bool message_in_buf(struct ledloard_client *c, const uint8_t *read_pos)
{
	size_t want, have;
	want = client_state_wants[c->state];
	have = c->write_idx - (read_pos - c->input_buffer);
	return want <= have;
}

int client_handle(struct ledloard_client *c)
{
	/* read some data and append it to our buffer */
	size_t buf_free = CLIENT_BUFFER_SIZE - c->write_idx;
	ssize_t n = read(c->fd, c->input_buffer + c->write_idx, buf_free);
	if (n == -1) {
		perror("read");
		return -1;
	}
	if (n == 0) {
		puts("client shutdown!");
		client_remove(c);
		return 1;
	}
	c->write_idx += n;

	/* read messages from input buffer */
	uint8_t *read_pos = c->input_buffer;
	while (message_in_buf(c, read_pos)) {
		enum ledloard_client_state saved_state = c->state;
		uint8_t type, prio;

		switch (c->state) {
		case IDLE:
			type = *read_pos;
			if (type == LB_TYPE_PRIO) {
				c->state = RECEIVING_PRIO;
			} else if (type == LB_TYPE_RAW) {
				c->state = RECEIVING_FRAME;
			} else {
				fprintf(stderr, "received unknown type %x!\n", type);
			}
			break;
		case RECEIVING_PRIO:
			prio = *read_pos;
			client_prio_set(c, prio);
			c->state = IDLE;
			break;
		case RECEIVING_FRAME:
			ledloard_frame_handle(c->loard, c, (uint8_t(*)[ARRAY_X_SIZE])read_pos);
			c->state = IDLE;
			break;
		}
		read_pos += client_state_wants[saved_state];
	}

	size_t nr_consumed = read_pos - c->input_buffer;
	size_t nr_buffered = c->write_idx;
	if (nr_consumed == nr_buffered) {
		/* everything read, reset write_idx */
		c->write_idx = 0;
	} else if (nr_consumed > 0) {
		/* copy incomplete data to start of buffer */
		memmove(c->input_buffer, read_pos, nr_buffered - nr_consumed);
		c->write_idx -= nr_consumed;
	}
	return 0;
}

/**
 * ledloard_run() - handle client connections
 */
int ledloard_run(struct ledloard *loard, int timeout)
{
	size_t i;
	int nr_active;

	nr_active = poll(loard->pollfds, loard->nr_pollfds, timeout);
	if (nr_active < 1)
		return nr_active; /* error (-1) or timeout (0) */

	/* check for new connections */
	if (loard->pollfds[0].revents) {
		nr_active--;
		int newfd = accept(loard->pollfds[0].fd, NULL, NULL);
		if (newfd == -1 || sock_nonblock(newfd) == -1)
			return -1;
		ledloard_client_add(loard, newfd);
		puts("accepted new connection!");
	}

	/* find out clients that want to be handled */
	for (i = 1; i < loard->nr_pollfds && nr_active > 0; i++) {
		if (loard->pollfds[i].revents) {
			nr_active--;
			client_handle(&loard->clients[i - 1]);
		}
	}

	return 0;
}

/**
 * ledloard_stop() - stop connection to ledloard, close network connections
 */
int ledloard_stop(struct ledloard *loard)
{
	size_t i;
	int err = 0;

	for (i = 0; i < loard->nr_pollfds; i++) {
		if (close(loard->pollfds[i].fd) == -1)
			err = -1;
	}
	/* reset old terminal attributes */
	if (tcsetattr(loard->led_fd, TCSANOW, &loard->term_backup) == -1)
		err = -1;
	if (close(loard->led_fd) == -1)
		err = -1;
	return err;
}

int main(int argc, char *argv[])
{
	struct ledloard loard;

	if (argc != 2) {
		puts("Usage: ledloard <serial device>");
		exit(1);
	}

	if (ledloard_init(&loard, argv[1], "1337") == -1) {
		perror("Unable to initialize ledloard");
		exit(1);
	}

	for (;;) {
		ledloard_run(&loard, 5000);
	}
	ledloard_stop(&loard);
	return 0;
}
