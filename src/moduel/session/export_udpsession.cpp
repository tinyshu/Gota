#include <string.h>
#include "export_udpsession.h"
#include "udp_session.h"

void export_udp_session::close() {

 }
void export_udp_session::send_data(unsigned char* pkg, int pkg_len) {

 }

void export_udp_session::send_msg(recv_msg* msg) {

 }

session_base* export_udp_session::get_inner_session() {
	return _udp_session;
}