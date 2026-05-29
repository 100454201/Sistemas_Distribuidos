# Distributed Systems – UC3M (2025-2026)

Coursework for the Distributed Systems course at Universidad Carlos III de Madrid.
All projects implemented in C and Python on Linux, following POSIX standards.

## Overview

This repository contains 4 graded exercises and a 2-part final project,
progressively building a complete distributed systems stack from scratch:
POSIX IPC → TCP Sockets → ONC-RPC → full messaging service.

## Graded Exercises

### Ejercicio Evaluable 1 — Key-Value Tuple Store (POSIX Message Queues)
Key-value store supporting tuples `<key, value1, value2, value3>`.
- **Part A**: Non-distributed version using a linked list + POSIX mutexes (`libclaves.so`)
- **Part B**: Distributed version using POSIX message queues (`servidor-mq.c` + `proxy-mq.c`)
- Concurrent server handling multiple clients simultaneously
- Full test suite passed on UC3M's Guernika server

### Ejercicio Evaluable 2 — Key-Value Tuple Store (TCP Sockets)
Same service as Exercise 1, reimplemented using TCP sockets.
- Language-independent application protocol defined between proxy and server
- Dynamic libraries: `libclaves.so` and `libproxyclaves.so`
- Concurrent server with environment variable configuration (`IP_TUPLAS`, `PORT_TUPLAS`)

### Ejercicio Evaluable 3 — Key-Value Tuple Store (ONC-RPC)
Same service reimplemented using ONC-RPC (`rpcgen`).
- XDR interface definition (`clavesRPC.x`)
- Auto-generated stubs integrated into the build system
- Server IP passed via `IP_TUPLAS` environment variable

### Ejercicio Evaluable 4 — Distributed Application Design
Design document for a distributed supermarket checkout monitoring system.
- 10 checkout terminals communicating with a central server via sockets
- Full application protocol specification: OPEN, SALE, CLOSE, STATUS operations
- Handles concurrent terminal updates and real-time monitoring

## Final Project — Messaging Service (WhatsApp-like)

### Part 1 — Core Messaging
Distributed messaging service with concurrent multithreaded server (C) and client (Python).
- Operations: REGISTER, UNREGISTER, CONNECT, DISCONNECT, SEND, USERS
- TCP socket communication protocol
- Offline message storage and delivery on reconnect
- Docker-compatible: clients and server run in separate containers

### Part 2 — Extended Features
- **File transfer** (SENDATTACH): peer-to-peer file sharing via sockets, bypassing the server
- **Web service**: Python message normalizer (REST) deployed locally on each client machine
- **ONC-RPC logging**: RPC server recording all user operations for audit trail

## Stack
C · Python · POSIX · TCP Sockets · ONC-RPC · Docker · Makefile · Linux
