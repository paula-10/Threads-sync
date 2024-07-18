// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h> // Add the pthread header

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t mutex;
pthread_mutex_t sum_mutex;
pthread_cond_t cond;

/* TODO: Define graph task argument. */
typedef struct {
	unsigned int idx;
} graph_task_arg_t;

void process_node(unsigned int idx);

void visit_neighbours(void *arg)
{
	unsigned int idx = ((graph_task_arg_t *)arg)->idx;
	os_node_t *node = graph->nodes[idx];

	for (unsigned int i = 0; i < node->num_neighbours; i++)
		if (graph->visited[node->neighbours[i]] == NOT_VISITED) {
			pthread_mutex_lock(&sum_mutex);
			graph->visited[node->neighbours[i]] = PROCESSING;
			pthread_mutex_unlock(&sum_mutex);
			process_node(node->neighbours[i]);
		}
}

void process_node(unsigned int idx)
{
	os_node_t *node;

	pthread_mutex_lock(&sum_mutex);

	node = graph->nodes[idx];
	sum += node->info;
	graph->visited[idx] = DONE;

	pthread_mutex_unlock(&sum_mutex);

	graph_task_arg_t *arg = malloc(sizeof(graph_task_arg_t));

	arg->idx = idx;

	os_task_t *task = create_task(visit_neighbours, arg, free);

	enqueue_task(tp, task);
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = (os_graph_t *)malloc(sizeof(os_graph_t));
	DIE(graph == NULL, "malloc");

	graph = create_graph_from_file(input_file);

	/* TODO: Initialize graph synchronization mechanisms. */
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&sum_mutex, NULL);
	pthread_cond_init(&cond, NULL);

	tp = create_threadpool(NUM_THREADS);
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);

	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&sum_mutex);
	pthread_cond_destroy(&cond);

	printf("%d", sum);

	return 0;
}
