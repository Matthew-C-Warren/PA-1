/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Matthew Warren
	UIN: 5533003350
	Date: 9/16/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;

double getSingleDataPoint(FIFORequestChannel& fifoChannel, int patientNum,  double timeSeconds,  int ecgNum) {
	char buf[MAX_MESSAGE]; // 256
    datamsg dataRequest(patientNum, timeSeconds, ecgNum);
	memcpy(buf, &dataRequest, sizeof(datamsg));
	fifoChannel.cwrite(buf, sizeof(datamsg)); // question
	double reply;
	fifoChannel.cread(&reply, sizeof(double));
	return reply;
}

void fetchPatientData(FIFORequestChannel& fifoChannel, int patientNum) {
	std::ofstream patientDataFile;
    patientDataFile.open ("received/x1.csv");
	for (double timeCounter = 0; timeCounter < 1000 * 0.004; timeCounter += 0.004) {
		double replyOne = getSingleDataPoint(fifoChannel, patientNum, timeCounter, 1);
		double replyTwo = getSingleDataPoint(fifoChannel, patientNum, timeCounter, 2);
		patientDataFile << timeCounter << "," << replyOne << "," << replyTwo << std::endl;
	}
	patientDataFile.close();
}

void requestFile(FIFORequestChannel& fifoChannel, string fileName, __int64_t offset, int length) {
	char buf[MAX_MESSAGE]; // 256
    filemsg fileSizeRequest(offset, length);
	int len = sizeof(filemsg) + fileName.size() + 1;
	memcpy(buf, &fileSizeRequest, sizeof(filemsg));
	strcpy(buf + sizeof(filemsg), fileName.c_str());
	fifoChannel.cwrite(buf, len);
}

void fetchFile(FIFORequestChannel& fifoChannel, string fileName) {

	string outputFileName = "received/" + fileName;
	// request file size
	requestFile(fifoChannel, fileName, 0, 0);
	// fetch file size
	__int64_t fileSize;
	fifoChannel.cread(&fileSize, sizeof(__int64_t));
	__int64_t currentIndex = 0;
	__int64_t currentLength = 0;
	__int64_t remainingLength = fileSize;

	FILE* outputFile;
	outputFile = fopen(outputFileName.c_str(), "wb");
	while (remainingLength > 0) {
		if (remainingLength <= MAX_MESSAGE) {
			currentLength = remainingLength;
			remainingLength = 0;
		} else {
			currentLength = MAX_MESSAGE;
			remainingLength -= MAX_MESSAGE;
		}
		requestFile(fifoChannel, fileName, currentIndex, currentLength);
		currentIndex += currentLength;

		char buf[MAX_MESSAGE];
		fifoChannel.cread(&buf, currentLength);
		fwrite(buf, 1, currentLength, outputFile);
	}
	fclose(outputFile);
}

void runClient(FIFORequestChannel& fifoChannel, int patientNum,  double timeSeconds,  int ecgNum, string fileName) {
	// run single data point fetch
	if (patientNum != -1 && timeSeconds != -1.0 && ecgNum!= -1) {
		double dataPoint = getSingleDataPoint(fifoChannel, patientNum, timeSeconds, ecgNum);
		cout << "For person " << patientNum << ", at time " << timeSeconds << ", the value of ecg " << ecgNum << " is " << dataPoint << endl;
	}

	// run patient data fetch
	if (patientNum != -1) {
		fetchPatientData(fifoChannel, patientNum);
	}

	if (fileName != ""){
		fetchFile(fifoChannel, fileName);
	}

}






int main (int argc, char *argv[]) {

	char* server_cmd[] = {(char*) "./server", nullptr};

	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	
	bool newChannel = false;
	string filename = "";
	string channelName="control";
	while ((opt = getopt(argc, argv, "p:t:e:f:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'c':
				newChannel = true;
				break;
		}

	}

	// open server as child process
	pid_t pid = fork();
    if (pid < 0) {
        return 1;
    }

    if (pid == 0) { //child process
        execvp(server_cmd[0], server_cmd); /// execute command ls
        return 0;
    }

	FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

	if (newChannel) {
		MESSAGE_TYPE m = NEWCHANNEL_MSG;
    	chan.cwrite(&m, sizeof(MESSAGE_TYPE));
		char buffer[MAX_MESSAGE];
		chan.cread(buffer, sizeof(buffer));
		channelName = string(buffer);
		FIFORequestChannel newChan(channelName, FIFORequestChannel::CLIENT_SIDE);
		runClient(newChan, p, t, e, filename);
		m = QUIT_MSG;
    	newChan.cwrite(&m, sizeof(MESSAGE_TYPE));
	} else {
		runClient(chan, p, t, e, filename);
	}	

	MESSAGE_TYPE m = QUIT_MSG;
   	chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	
    // // sending a non-sense message, you need to change this
	// filemsg fm(0, 0);
	// string fname = "teslkansdlkjflasjdf.dat";
	
	// int len = sizeof(filemsg) + (fname.size() + 1);
	// char* buf2 = new char[len];
	// memcpy(buf2, &fm, sizeof(filemsg));
	// strcpy(buf2 + sizeof(filemsg), fname.c_str());
	// chan.cwrite(buf2, len);  // I want the file length;

	// delete[] buf2;
	
	// closing the channel    

}