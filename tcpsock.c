/**********************************************************
	Modify History: qiuzhb make, 2016-8-21
**********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>

#include "tcpsock.h"


/* create a tcp socket with server mode
   client_port(opt)
   return > 0(socket fd), sucess
   return -1, error */
int open_client_port(unsigned short client_port)
{
	int sock;
	struct sockaddr_in client_addr;
	int opt_val = 1;

	//create tcp ssocket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		TCP_SOCKET_ERR("create client tcp socket error\n");
		return -1;
    }

	//set reuse of local addresses
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) == -1){
		TCP_SOCKET_ERR("setsockopt client tcp socket error\n");
		goto socket_exit;
	}

	//bind port
	if(client_port > 0){
		//bind
		memset(&client_addr, 0, sizeof(client_addr));
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = htonl(INADDR_ANY);;  /* any interface */
		client_addr.sin_port = htons(client_port);
	    if(bind(sock, (struct sockaddr*)&client_addr, sizeof(client_addr)) == -1){
			TCP_SOCKET_ERR("bind client tcp socket error\n");
			goto socket_exit;
	    }
	}
	return sock;

socket_exit:
	close(sock);
	return -1;
	
}


/* set socket non-blocking
   return 0, sucess
   return -1, error */
int set_socket_non_blocking(int sock)
{
	int flags;

	if((flags = fcntl(sock, F_GETFL, 0)) == -1){
		TCP_SOCKET_ERR("Get the file access mode and the file status flags error\n");
		return -1;
	}

	if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1){
		TCP_SOCKET_ERR("set O_NONBLOCK flags error\n");
		return -1;
	}
	return 0;
}

/* connection server
   err_call_bk(opt)
   return < 0, error
   return = 0, sucess*/
int connection_server(int client_sock, char* server_num_and_dot_IPv4, 
	unsigned short server_port)
{
	int ret;
	struct sockaddr_in serv_addr;
	in_addr_t addr;

	if(client_sock <= 0){
		TCP_SOCKET_ERR("error client_sock:%d\n", client_sock);
		return -1;
	}
	if(server_port < 0){
		TCP_SOCKET_ERR("error server_port:%d\n", server_port);
		return -1;
	}
	if(server_num_and_dot_IPv4){
		addr = inet_addr(server_num_and_dot_IPv4);
		if(addr == -1){
			TCP_SOCKET_ERR("ip address error:%s\n", server_num_and_dot_IPv4);
			return -1;
		}
	}else{
		TCP_SOCKET_ERR("server_num_and_dot_IPv4 error\n");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(server_port);
	serv_addr.sin_addr.s_addr = addr;
	if((ret = connect(client_sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in))) < 0){
		TCP_SOCKET_ERR("connect error(%d:%s)\n", ret, strerror(errno));
		return ret;
	}
	
	return 0;
}		
	  
// do close
int close_connection(int sock)
{
    if(close(sock) == -1){
		TCP_SOCKET_ERR("close socket error\n");
		return -1;
    }
	return 0;
}

/* return < 0, error
   return > 0,(send data len) sucess*/
int send_packet(int sock, void *buf, int len, int is_no_wait)
{
	int ret;
	int flag;
	
	if(sock <= 0){
		TCP_SOCKET_ERR("error sock:%d\n", sock);
		return -1;
	}
	if(buf == NULL){
		TCP_SOCKET_ERR("error buf:%lx\n", (unsigned long)buf);
		return -1;
	}
	if(len < 0){
		TCP_SOCKET_ERR("error len:%d\n", len);
		return -1;
	}

	//set blocking
	if(is_no_wait > 0){
		flag = MSG_DONTWAIT;
	}else{
		flag = 0;
	}

	if((ret = send(sock, buf, len, flag)) < 0){
		TCP_SOCKET_ERR("send packet error(%d:%s)\n", ret, strerror(errno));
		return ret;
	 }

	return ret;
}

/* is_no_wait(opt, if > 0 wait until message received)
   err_call_bk(opt)
   return 0, connect has been close by remote side
   return < 0, error
   return > 0,(received data len) sucess*/
int receive_packet(int sock, void *buf, int len, int is_no_wait)
{
	int ret;
	int flag;
	
	if(sock <= 0){
		TCP_SOCKET_ERR("error sock:%d\n", sock);
		return -1;
	}
	if(buf == NULL){
		TCP_SOCKET_ERR("error buf:%lx\n", (unsigned long)buf);
		return -1;
	}
	if(len < 0){
		TCP_SOCKET_ERR("error len:%d\n", len);
		return -1;
	}

	//set blocking
	if(is_no_wait > 0){
		flag = MSG_DONTWAIT;
	}else{
		flag = 0;
	}

	if((ret = recv(sock, buf, len, flag)) < 0){
		TCP_SOCKET_ERR("send packet error(%d:%s)\n", ret, strerror(errno));
		return ret;
	}else if(ret == 0){
		TCP_SOCKET_ERR("the connect has been close by remote side.(%d:%s)\n", ret, strerror(errno));
		return 0;
	}else{
		return ret;
	}
}



/* do select 
opt, for select option
return 0, timeout
return < 0, error
return > 0, sucess
*/
int select_socket(int sock, int ms_timeout, SELECT_OPT opt)
{
    int ret;
    struct timeval *time_out, timer;
    fd_set rfds, wfds, efds;
	fd_set *p_rfds = NULL, *p_wfds = NULL, *p_efds = NULL;

	if(sock <= 0){
		TCP_SOCKET_ERR("error sock:%d\n", sock);
		return -1;
	}

	if(ms_timeout >= 0){
	    timer.tv_sec = ms_timeout / 1000; 
	    timer.tv_usec = (ms_timeout % 1000) * 1000;
		time_out = & timer;
	}else{
		time_out = NULL;
	}

	/**** clear the fdset then set the socket in the fdset for selection ********/
	switch(opt){
		case SELECT_READ:
			FD_ZERO(&rfds);
			FD_SET(sock, &rfds);
			p_rfds = &rfds;
			break;
		case SELECT_WRITE:
			FD_ZERO(&wfds);
			FD_SET(sock, &wfds);
			p_wfds = &wfds;
			break;
		case SELECT_EXCEPTION:
			FD_ZERO(&efds);
			FD_SET(sock, &efds);
			p_efds = &efds;
			break;
		case SELECT_FOR_SLEEP:
			//do nothing
			break;
		default:
			return -1;
	}

	//printf("start\n");
    ret = select(sock+1, p_rfds, p_wfds, p_efds, time_out);
	//printf("end\n");
	if(ret < 0){
		/**** select error ******/
		TCP_SOCKET_ERR("select error(%d:%s)\n", ret, strerror(errno));
		return ret;
	}else if(ret == 0){
		if(opt == SELECT_FOR_SLEEP){
			//for sucess sleep
			return 0;
		}else{
			/**** select timeout *********/
			TCP_SOCKET_ERR("select timer out!\n");
			return 0;
		}
	}else if(ret == 1){
		return sock;
	}else{
		TCP_SOCKET_ERR("select error , ret = %d\n", ret);
		return -1;
	}
}

/*
return < 0, error 
return = 0, sucess
*/
int clear_sock_receive_buffer(int sock)
{
	int ret;
	char tmp;
	
	if(sock < 0){
		TCP_SOCKET_ERR("sock error , %d\n", sock);
		return -1;
	}

	while(1){
		ret = select_socket(sock, 0, SELECT_READ);
		if(ret > 0){
			recv(sock, &tmp, 1, 0);
		}else if(ret == 0){
			break;
		}else{
			return -1;
		}
	}
	return 0;
}


