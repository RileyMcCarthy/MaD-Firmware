#include <stdint.h>

void socketio_begin();
int32_t socketio_create_socket(int32_t rxpin);
void socketio_send(int32_t socket_id, uint8_t data);
bool socketio_poll(int32_t socket_id);
int32_t socketio_receive(int32_t socket_id, uint8_t *data, uint16_t length);
void socketio_set_address(char *address, int port);
void socketio_send_str(int32_t socket_id, char *data);
void socketio_close(int32_t socket_id);
