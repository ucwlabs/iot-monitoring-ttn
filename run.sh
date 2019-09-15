#!/bin/bash

DOCKER_COMPOSE_CMD=docker-compose
DATA_DIR=${PWD}/storage

mkdir -p $DATA_DIR
mkdir -p $DATA_DIR/influxdb
mkdir -p $DATA_DIR/grafana

chmod -R 777 $DATA_DIR

env DATA_DIR=$DATA_DIR $DOCKER_COMPOSE_CMD build ttn-bridge
env DATA_DIR=$DATA_DIR $DOCKER_COMPOSE_CMD up -d