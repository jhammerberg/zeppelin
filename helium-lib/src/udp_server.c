#include <helium/udp_server.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

#define UDP_ECHO_PORT 4242
#define UDP_ECHO_RECV_BUF_SIZE 256

#define UDP_ECHO_STACK_SIZE 2048
#define UDP_ECHO_THREAD_PRIORITY 5

// Static storage for the dedicated UDP echo server thread
static K_THREAD_STACK_DEFINE(udp_echo_stack, UDP_ECHO_STACK_SIZE);
static struct k_thread udp_echo_thread;

// Bind a UDP socket to the given port on all interfaces (IPv4)
static int udp_echo_bind(int sock, uint16_t port) {
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (zsock_bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printk("Failed to bind UDP socket: %d\n", errno);
        return -1;
    }

    return 0;
}

// Blocking echo loop: receive a datagram and send it back to the sender
static void udp_echo_loop(int sock) {
    uint8_t buf[UDP_ECHO_RECV_BUF_SIZE];

    while (1) {
        struct sockaddr src_addr;
        socklen_t src_addr_len = sizeof(src_addr);

        ssize_t received =
            zsock_recvfrom(sock, buf, sizeof(buf), 0, &src_addr, &src_addr_len);
        if (received < 0) {
            printk("UDP recvfrom error: %d\n", errno);
            continue;
        }

        ssize_t sent =
            zsock_sendto(sock, buf, received, 0, &src_addr, src_addr_len);
        if (sent < 0) {
            printk("UDP sendto error: %d\n", errno);
            continue;
        }

        printk("Echoed %d bytes\n", (int)sent);
    }
}

// Thread entry point: create the socket, bind it, then serve forever
static void udp_echo_thread_entry(void* arg1, void* arg2, void* arg3) {
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    int sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printk("Failed to create UDP socket: %d\n", errno);
        return;
    }

    if (udp_echo_bind(sock, UDP_ECHO_PORT) < 0) {
        zsock_close(sock);
        return;
    }

    printk("UDP echo server listening on port %d\n", UDP_ECHO_PORT);
    udp_echo_loop(sock);

    zsock_close(sock);
}

// Spawn the dedicated UDP echo server thread
k_tid_t udp_echo_server_start(void) {
    return k_thread_create(&udp_echo_thread, udp_echo_stack,
                           K_THREAD_STACK_SIZEOF(udp_echo_stack),
                           udp_echo_thread_entry, NULL, NULL, NULL,
                           UDP_ECHO_THREAD_PRIORITY, 0, K_NO_WAIT);
}
