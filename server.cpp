#include <thread>
#include "TCPRequestChannel.h"

using namespace std;


int buffercapacity = MAX_MESSAGE;
char* buffer = NULL; // buffer used by the server, allocated in the main

int nchannels = 0;
vector<string> all_data[NUM_PERSONS];


void populate_file_data (int person) {
	//cout << "populating for person " << person << endl;
	string filename = "BIMDC/" + to_string(person) + ".csv";
	char line[100];
	ifstream ifs(filename.c_str());
	if (ifs.fail()){
		EXITONERROR("Data file: " + filename + " does not exist in the BIMDC/ directory");
	}
	while (!ifs.eof()) {
		line[0] = 0;
		ifs.getline(line, 100);
		if (ifs.eof()) {
			break;
		}
		
		if (line[0]) {
			all_data[person-1].push_back(string(line));
		}
	}
}

double get_data_from_memory (int person, double seconds, int ecgno) {
	int index = (int) round(seconds / 0.004);
	string line = all_data[person-1][index]; 
	vector<string> parts = split(line, ',');
	
	double ecg1 = stod(parts[1]);
	double ecg2 = stod(parts[2]); 
	if (ecgno == 1) {
		return ecg1;
	}
	else {
		return ecg2;
	}
}

void process_file_request (TCPRequestChannel* rc, char* request) {
	filemsg f = *((filemsg*) request);
	string filename = request + sizeof(filemsg);
	filename = "BIMDC/" + filename; // adding the path prefix to the requested file name
	//cout << "Server received request for file " << filename << endl;

	if (f.offset == 0 && f.length == 0) { // means that the client is asking for file size
		__int64_t fs = get_file_size (filename);
		rc->cwrite ((char*) &fs, sizeof(__int64_t));
		return;
	}

	/* request buffer can be used for response buffer, because everything necessary have
	been copied over to filemsg f and filename*/
	char* response = request; 

	// make sure that client is not requesting too big a chunk
	if (f.length > buffercapacity) {
		cerr << "Client is requesting a chunk bigger than server's capacity" << endl;
		cerr << "Returning nothing (i.e., 0 bytes) in response" << endl;
		rc->cwrite(response, 0);
	}

	FILE* fp = fopen(filename.c_str(), "rb");
	if (!fp) {
		cerr << "Server received request for file: " << filename << " which cannot be opened" << endl;
		rc->cwrite(buffer, 0);
		return;
	}
	fseek(fp, f.offset, SEEK_SET);
	int nbytes = fread(response, 1, f.length, fp);

	/* making sure that the client is asking for the right # of bytes,
	this is especially imp for the last chunk of a file when the 
	remaining lenght is < buffercap of the client*/
	assert(nbytes == f.length); 

	rc->cwrite(response, nbytes);
	fclose(fp);
}

void process_data_request (TCPRequestChannel* rc, char* request) {
	datamsg* d = (datamsg*) request;
	double data = get_data_from_memory(d->person, d->seconds, d->ecgno);
	rc->cwrite(&data, sizeof(double));
}

void process_unknown_request (TCPRequestChannel* rc) {
	char a = 0;
	rc->cwrite(&a, sizeof(char));
}


void process_request (TCPRequestChannel* rc, char* _request) {
	MESSAGE_TYPE m = *((MESSAGE_TYPE*) _request);
	if (m == DATA_MSG) {
		usleep(rand() % 5000);
		process_data_request(rc, _request);
	}
	else if (m == FILE_MSG) {
		process_file_request(rc, _request);
	}
	else {
		process_unknown_request(rc);
	}
}

void handle_process_loop (TCPRequestChannel* channel) {
	/* creating a buffer per client to process incoming requests
	and prepare a response */
	char* buffer = new char[buffercapacity];
	if (!buffer) {
		EXITONERROR ("Cannot allocate memory for server buffer");
	}

	while (true) {
		int nbytes = channel->cread(buffer, buffercapacity);
		if (nbytes < 0) {
			cerr << "Client-side terminated abnormally" << endl;
			break;
		}
		else if (nbytes == 0) {
			cout << "Server could not read anything... Terminating" << endl;
			break;
		}

		if (nbytes >= sizeof(MESSAGE_TYPE)) {
            MESSAGE_TYPE m;
            memcpy(&m, buffer, sizeof(MESSAGE_TYPE));  // Read only the bytes for MESSAGE_TYPE
            // Process the message based on its type
            if (m == QUIT_MSG) {
                cout << "Client-side is done and exited" << endl;
                break;
            }
		}
		process_request(channel, buffer);
	}
	delete[] buffer;
	delete channel;
}

int main (int argc, char* argv[]) {
	buffercapacity = MAX_MESSAGE;
	int opt;
	string port = "";
	while ((opt = getopt(argc, argv, "m:r:")) != -1) {
		switch (opt) {
			case 'm':
				buffercapacity = atoi(optarg);
				break;
			case 'r':
				port = optarg;
				break;
		}
	}

	if (port.empty()) {
        cerr << "No port number provided" << endl;
        exit(EXIT_FAILURE);
    }

	srand(time_t(NULL));
	for (int i = 0; i < NUM_PERSONS; i++) {
		populate_file_data(i+1);
	}
	
	// FIXME: open socket with address="" and r from CLI
	//		  then enter infinite loop calling TCPReqChan::accept_conn() and 2nd TCP constructor
	//		  dispatching handle_process_loop thread for new channel
	
	// Call the constructor to create server-side control channel
	
	TCPRequestChannel server("", port);
	while (true) {
		// Accept a new connection socket
		// Create a new channel with the accepted socket (the socket is already established)
		// Dispatch a new thread to handle_procces_loop
		// Since the main thread is not blocked, it can accept more connections (ise detach instead of join)
		//use setsockopt ?

	    int newsockfd = server.accept_conn();
        if (newsockfd < 0) {
            cerr << "Error accepting connection" << endl;
            continue; // Continue accepting other connections
        }
        TCPRequestChannel* workerChannel = new TCPRequestChannel(newsockfd);
        thread worker(handle_process_loop, workerChannel);
        worker.detach(); // Detach the thread to handle the connection independently
	}

	cout << "Server terminated unexpectedly" << endl;
	//delete control_channel;
}
