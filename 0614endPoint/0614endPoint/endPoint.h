#pragma once


extern int client_end_point();

extern int server_end_point();

extern int create_tcp_socket();

extern int create_acceptor_socket();

extern int bind_acceptor_socket();

extern int connect_to_end();

extern int dns_connect_to_end();

extern int accept_new_connection();

extern void use_const_buffer();



extern void use_buffer_str();

extern void use_buffer_array();

extern void use_stream_buffer();



extern void write_to_socket(boost::asio::ip::tcp::socket& sock);

extern int send_data_by_write_some();

extern int send_data_by_send();

extern int send_data_by_write();



extern std::string read_from_socket(boost::asio::ip::tcp::socket& sock);

extern int read_data_by_read_some();

extern int read_data_by_receive();

extern int read_data_by_read();

extern std::string read_data_by_until(boost::asio::ip::tcp::socket& sock);