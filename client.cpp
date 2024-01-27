// your PA3 client code here
#include <fstream>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/wait.h>

#include "BoundedBuffer.h"
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "TCPRequestChannel.h"

// ecgno to use for datamsgs
#define ECGNO 1

using namespace std;


struct PVPair {
	int p;
	double v;
	
	PVPair (int _p, double _v) : p(_p), v(_v) {}
};

TCPRequestChannel* create_new_channel(TCPRequestChannel* chan, int b){
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    chan->cwrite(&m, sizeof(MESSAGE_TYPE));

    unique_ptr<char[]> chname = make_unique<char[]>(b);
    chan->cread(chname.get(), b);

    string new_channel(chname.get(), b);

    // open channel on client side                  (chname.get(), FIFORequestChannel::CLIENT_SIDE)
    //TCPRequestChannel* nchan = new TCPRequestChannel(new_channel, chname);

    return chan;
}

void patient_thread_function (int n, int pno, BoundedBuffer* rb) {
    datamsg d(pno, 0.0, ECGNO);
    // populate data messages
    for (int i = 0; i < n; i++) {
        rb->push((char*) &d, sizeof(datamsg));
        d.seconds += 0.004;
    }
}

void file_thread_function (string fname, TCPRequestChannel* chan, int b, BoundedBuffer* rb) {
    // open receiving fi    le and truncate appropriate number of bytes
    filemsg f(0, 0);
    size_t bufsize = sizeof(filemsg) + fname.size() + 1;
    unique_ptr<char []> buf = make_unique<char []>(bufsize);
    memcpy(buf.get(), &f, sizeof(filemsg));
    strcpy(buf.get() + sizeof(filemsg), fname.c_str());
    
    chan->cwrite(buf.get(), bufsize);
	
    __int64_t flen;
    chan->cread(&flen, sizeof(__int64_t));
    
    FILE* myfile = fopen(("received/" + fname).c_str(), "wb");
    fseek(myfile, flen, SEEK_SET);
    fclose(myfile);

    // populate file messages
	__int64_t remainingbytes = flen;
    int buffer = b;
    int offset = 0;
    filemsg* fm = (filemsg*) buf.get();
    while (remainingbytes > 0) {
        if (buffer > remainingbytes) {
            buffer = remainingbytes;
        }
        offset = flen - remainingbytes;

        fm->offset = offset;
        fm->length = buffer;
        rb->push(buf.get(), bufsize);

        remainingbytes -= buffer;
    }
}

void worker_thread_function (TCPRequestChannel* chan, BoundedBuffer* rb, BoundedBuffer* sb, int b) {
    unique_ptr<char[]> buf = make_unique<char[]>(b);
    double result = 0.0;
    unique_ptr<char[]> recvbuf = make_unique<char[]>(b);
    
    while (true) {
        rb->pop(buf.get(), b);
        MESSAGE_TYPE* m = (MESSAGE_TYPE*) buf.get();

        if (*m == DATA_MSG) {
            chan->cwrite(buf.get(), sizeof(datamsg));
            chan->cread(&result, sizeof(double));
			PVPair pv(((datamsg*) buf.get())->person, result);
            sb->push((char*) &pv, sizeof(PVPair));
        }
        else if (*m == FILE_MSG) {
            filemsg* fm = (filemsg*) buf.get();
            string fname = (char*) (fm + 1);
            chan->cwrite(buf.get(), (sizeof(filemsg) + fname.size() + 1));
			chan->cread(recvbuf.get(), fm->length);

            FILE* myfile = fopen(("received/" + fname).c_str(), "rb+");
			fseek(myfile, fm->offset, SEEK_SET);
            fwrite(recvbuf.get(), 1, fm->length, myfile);
            fclose(myfile);
        }
        else if (*m == QUIT_MSG) {
            chan->cwrite(m, sizeof(MESSAGE_TYPE));
            delete chan;
            break;
        }
    }
}

void histogram_thread_function (BoundedBuffer* rb, HistogramCollection* hc) {
	char buf[sizeof(PVPair)];
	while (true) {
		rb->pop(buf, sizeof(PVPair));
		PVPair pv = *(PVPair*) buf;
		if (pv.p <= 0) {
			break;
		}
		hc->update(pv.p, pv.v);
	}
}


int main (int argc, char *argv[]) {
    int n = 1000;	// default number of requests per "patient"
    int p = 10;		// number of patients [1,15]
    int w = 100;	// default number of worker threads
	int h = 20;		// default number of histogram threads
    int b = 30;		// default capacity of the request buffer
	int m = MAX_MESSAGE;	// default capacity of the message buffer
	string f = "";	// name of file to be transferred
    string server_ip = "";
    string server_port = "";
    
    // read arguments
    int opt;
	while ((opt = getopt(argc, argv, "n:p:w:h:b:m:f:a:r:")) != -1) {
		switch (opt) {
			case 'n':
				n = atoi(optarg);
                break;
			case 'p':
				p = atoi(optarg);
                break;
			case 'w':
				w = atoi(optarg);
                break;
			case 'h':
				h = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
                break;
			case 'm':
				m = atoi(optarg);
                break;
			case 'f':
				f = optarg;
                break;
            case 'a':
				server_ip = optarg;
                break;
            case 'r':
				server_port = optarg;
                break;
		}
	}
    bool filereq = (f != "");
    
    
	// control overhead (including the control channel)
	//TCPRequestChannel* chan = new TCPRequestChannel(server_ip, server_port);
    //unique_ptr<TCPRequestChannel> chan = make_unique<TCPRequestChannel>(server_ip, server_port);
    TCPRequestChannel* chan = new TCPRequestChannel(server_ip, server_port);
    BoundedBuffer request_buffer(b);
    BoundedBuffer response_buffer(b);
	HistogramCollection hc;
    

    // making histograms and adding to collection
    for (int i = 0; i < p; i++) {
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }

    // making worker channels
    //vector<TCPRequestChannel *> worker_channels;
    unique_ptr<TCPRequestChannel*[]> wchans = make_unique<TCPRequestChannel*[]>(w);
    for (int i = 0; i < w; i++) {
        wchans[i] = new TCPRequestChannel(server_ip, server_port);
    }
	
	// record start time
    struct timeval start, end;
    gettimeofday(&start, 0);

    /* Start all threads here */ 
    unique_ptr<thread[]> patients = make_unique<thread[]>(p);
    thread file;
    if (!filereq) {
        for (int i = 0; i < p; i++) {
            patients[i] = thread(patient_thread_function, n, i+1, &request_buffer);
        }
    }
    else {
        // ERROR IS CAUSED HERE
        file = thread(file_thread_function, f, chan, m, &request_buffer);
    }

    // COMMENTED OUT FOR DEBUGGING
    unique_ptr<thread[]> workers = make_unique<thread[]>(w);
    for (int i = 0; i < w; i++) {
        workers[i] = thread(worker_thread_function, wchans[i], &request_buffer, &response_buffer, m);
    }

    unique_ptr<thread[]> hists = make_unique<thread[]>(h);
    for (int i = 0; i < h; i++) {
        hists[i] = thread(histogram_thread_function, &response_buffer, &hc);
    }
	
	/* Join all threads here */
    if (!filereq) {
        for (int i = 0; i < p; i++) {
            patients[i].join();
        }
    }
    else {
        file.join();
    }

    for (int i = 0; i < w; i++) {
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char*) &q, sizeof(MESSAGE_TYPE));
    }

    // COMMENTED OUT FOR DEBUGGING
    for (int i = 0; i < w; i++) {
        workers[i].join();
    }
	
	for (int i = 0; i < h; i++) {
		PVPair pv(0, 0);
		response_buffer.push((char*) &pv, sizeof(PVPair));
	}
	
	for (int i = 0; i < h; i++) {
		hists[i].join();
	}

	// record end time
    gettimeofday(&end, 0);

    // print the results
	if (f == "") {
		hc.print();
	}
    int secs = ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int) 1e6);
    int usecs = (int) ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

	// quit and close control channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!" << endl;
    delete chan;

	// wait for server to exit  <-- not needed bcauser server/client not a child process 
	wait(nullptr);

    // clean up ????
    // delete patient_threads;
    // delete worker_threads;
    // delete histogram_threads;
    // delete chan;
}
