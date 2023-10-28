#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/ip.h> 
#include <unistd.h>
#include <fstream>
#include <pthread.h>
#include <vector>
#include <sys/stat.h>
#include <ctype.h>
#include <math.h>
#include <unordered_map>
#include <openssl/sha.h>
#include <openssl/evp.h>  
#include <dirent.h>  

using namespace std;

#define bufferSize 32*1024 //divides 512kb in 16 parts

string trackerip="127.0.0.1"; //tracker ip from file
int trackerport=8080; //tracker port from file
string myId;
string trackerFileName="tracker_info.txt";
string logFileName="logs.txt";

vector<pthread_t> threadsRecv;
vector<pthread_t> threadsSend;
string myIp;
string myPort;

unordered_map<string,string> files;

void logIntoFile(string logstring)
{
	ofstream fout;
	fout.open((myId+"/"+logFileName),ios::app);
	fout<<"\n"<<logstring;
}

long long getFilesize(string filename) {
    struct stat st;
    if(stat((myId+"/Uploads/"+filename).c_str(), &st) != 0) {
        return 0;
    }
    return st.st_size;   
}

void createDir(string filename)
{
	mkdir(filename.c_str(),0777);
	mkdir((filename+"/Uploads").c_str(),0777);
	mkdir((filename+"/Downloads").c_str(),0777);
	return;
}

void updateBitmap()
{
	ifstream bitmapfile;
	bitmapfile.open((myId+"/bitmap.txt").c_str());
	string filenames,bitmap;
	while(bitmapfile >> filenames >> bitmap)
	{
		files[filenames]=bitmap;
	}
	bitmapfile.close();
}

void postBitmap()
{
	ofstream bitmapfile;
	bitmapfile.open((myId+"/bitmap.txt").c_str());
	string bitmap;
	for(auto i:files)
	{
		bitmap = i.first+" "+i.second+"\n";
		bitmapfile<<bitmap;
	}
	bitmapfile.close();
}

string getShaHash(string filename)
{
	string ans="";
	char buffer[bufferSize];
	int len;
	SHA_CTX ctx;
	SHA1_Init(&ctx);
	FILE* inputFile = fopen((filename).c_str(),"rb");
		
	while((len = fread(buffer,1,bufferSize, inputFile)) > 0)
	{
		SHA1_Update(&ctx, buffer, len);
		bzero(buffer,bufferSize);
		
	}
	
	fclose(inputFile);
	unsigned char hash[SHA_DIGEST_LENGTH];
	SHA1_Final(hash, &ctx);	
	for(int i = 0; i < 20; i++){  
        // ans+=hash[i];  
        sprintf(buffer + (i << 3), "%08x", hash[i]);
    }
    ans=buffer;
    return ans;

}


int processInputCommand(string command)
{
	vector<string> args;
	string str="";
	for(int i=0;i<command.length();i++)
	{
		if (command[i]==' ')
		{
			args.push_back(str);
			str="";
			
		}
		else
		{
			str+=command[i];
		}
	}
	args.push_back(str);
	if((args[0]=="create_user"&&myId=="") || args[0]=="login")
	{
		createDir(args[1]);
		myId=args[1];
	}
	if(args[0]=="login")
	{
		//get updated bitmap from file
		updateBitmap();
		
	}

	if(args[0]=="download")
		return 1;
	else if(args[0]=="upload_file")
		return 2;
	else if(args[0]=="logout")
	{

		postBitmap();
		
		return 0;
	}
	else if(args[0]=="hash")
	{
		return 3;
	}
	else return 0;

}

vector<string> getCommandArgs(string command)
{
	string str="";
	vector<string> args;
	for(int i=0;i<command.length();i++)
	{
		if (command[i]==' ')
		{
			args.push_back(str);
			str="";
			
		}
		else
		{
			str+=command[i];
		}
	}
	args.push_back(str);
	return args;
}

vector<pair<string,string> > getTrackerInfo()
{
	vector<pair<string,string> > result;
	string num,ip,port;
	ifstream trackerFile;
	trackerFile.open(trackerFileName.c_str());
	while(trackerFile >> num >> ip >> port)
	{
		result.push_back(make_pair(ip,port));	
	}
	return result;
}

string getBitMap(long long numberOfChunks)
{
	string ans="";
	for(int i=0;i<numberOfChunks;i++)
	{
		ans+='1';
	}
	return ans;
}
bool fileExists(string filename)
{
	ifstream infile((myId+"/Uploads/"+filename).c_str());
	return infile.good();
}


string sendDataToTracker(string command)
{
	command+=" "+myId+" "+myIp+" "+myPort;
	int commandType = processInputCommand(command);
	if(commandType==2) //upload
	{
		vector<string> commandArgs = getCommandArgs(command);

		if(!fileExists(commandArgs[1]))
		{
			logIntoFile("[-] Upload failed because file is not available\n");
			return "File not available";
		}
		long long size = getFilesize(commandArgs[1]);
		long long numberOfChunks = ceil((size*1.0)/(512*1024));
		command+=" "+to_string(numberOfChunks);
		command+=" "+getShaHash(myId+"/Uploads/"+commandArgs[1]);
		files[commandArgs[1]]=getBitMap(numberOfChunks);
	}
	else if(commandType==3)
	{
		vector<string> commandArgs = getCommandArgs(command);
		string newCommand = commandArgs[1]+" "+commandArgs[2]+" "+commandArgs[3]+" "+commandArgs[5]+" "+commandArgs[6]+" "+commandArgs[7]+" 1 "+commandArgs[4];
		command=newCommand;
	}
	//writing in command list for sync
	ofstream commandFile("commands.txt",ios_base::app);
	commandFile<<command+"\n";
	
	int anothersocketfd = socket(AF_INET,SOCK_STREAM,0);
	
	char buffer[bufferSize];int n;
	struct hostent *tracker;
	vector<pair<string,string> > trackerInfo = getTrackerInfo();
	int flag=0;
	int flag2=0;
	int flag3=0;
	for(int i=0;i<trackerInfo.size();i++)
	{		
		anothersocketfd = socket(AF_INET,SOCK_STREAM,0);
		struct sockaddr_in trackerAddr;
		trackerip = trackerInfo[i].first;
		trackerport = atoi(trackerInfo[i].second.c_str());
	  	tracker = gethostbyname(trackerip.c_str());
	  	if(tracker==NULL)
	  	{
	  		flag=-1;
	  		continue;
	  	}
	  	else
	  	{
	  		flag=1;
	  		bzero((char*)&trackerAddr,sizeof(trackerAddr));
			trackerAddr.sin_family =  AF_INET;

			bcopy((char*)tracker->h_addr,(char*)&trackerAddr.sin_addr.s_addr,tracker->h_length);
			trackerAddr.sin_port = htons(trackerport);
			if(connect(anothersocketfd,(struct sockaddr*)&trackerAddr,sizeof(trackerAddr)) < 0)
			{	
				flag2=-1;		
				continue;
			}
			else
			{
				
				flag2=1;
				bzero(buffer,bufferSize);
				sprintf(buffer, "%s", command.c_str());
				n = write(anothersocketfd,buffer,bufferSize);
				if(n<0)
				{
					cout<<"Error in sending request from peer!\n";
					
				}	
				bzero(buffer,bufferSize);
				n = read(anothersocketfd,buffer,bufferSize);
				if(n<0)
				{
					cout<<"Error in reading from tracker!\n";
					
				}
				string ele=buffer;
				if(ele!="" && flag3==0)
				{
					flag3=1;
					cout<<"Tracker:"<<buffer<<endl;
				}

				close(anothersocketfd);
				// break;
			}
	  	}
	  	
	}
	if(flag==-1 || flag==-2)
	{
		logIntoFile("[-] Wrong tracker info or tracker down!\n");
		cout<<"Wrong tracker info or tracker down!\n";
	}
	commandFile.close();
	return buffer;
	bzero(buffer,bufferSize);
}

void* sendTheData(void* sfd)
{
	int newSocketfd =  *((int*)sfd);
	char buffer[bufferSize];int n;
	bzero(buffer,bufferSize);
	
	n = read(newSocketfd,buffer,bufferSize);
	if(n<0)
	{
		logIntoFile("[-] Error in reading input file name!\n");
	}
	
	string inputFileName = buffer;
	string chunkstring;

	vector<string> inputFileNameArgs = getCommandArgs(inputFileName);
	if(inputFileNameArgs[0]=="bitmap")
	{
		bzero(buffer,bufferSize);
		//send bitmap for requested file
		sprintf(buffer, "%s", files[inputFileNameArgs[1]].c_str()); // cuz 0 index will be "bitmap"
		n = write(newSocketfd,buffer,bufferSize);
		if(n<0)
		{
			logIntoFile("[-] Error in sending bitmap!\n");
		}

	 }	
	else{
		int chunkNum = atoi(inputFileNameArgs[1].c_str());
		inputFileName=inputFileNameArgs[0];

		FILE* inputFile = fopen((myId+"/Uploads/"+inputFileName).c_str(),"rb");
		fseek(inputFile, chunkNum*512*1024 ,SEEK_SET );
		bzero(buffer,bufferSize);
		int c=0;
		int len;
		while((len = fread(buffer,1,bufferSize, inputFile)) > 0 && c<16)
		{
			
			while(len<32768 )
			{
				if(inputFileNameArgs[2]=="last") break;
				else{
					bzero(buffer,bufferSize);
					fseek(inputFile, -1*len ,SEEK_CUR );
					len = fread(buffer,1,bufferSize, inputFile);
				}
			}
			send(newSocketfd,buffer,len,0);
			bzero(buffer,bufferSize);
			
			c++;
		}
		
		fclose(inputFile);
	}
	shutdown(newSocketfd,2);
	close(newSocketfd);
	return NULL;
}

void* handleSendingDataThread(void* sfd)
{
	int socketfd =  *((int*)sfd);
	
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen;
	listen(socketfd,100);
	clientAddrLen = sizeof(clientAddr);

	while(1)
	{
		int newSocketfd = accept(socketfd,(struct sockaddr*)&clientAddr,&clientAddrLen);
		if(newSocketfd<0) 
		{
			logIntoFile("[-] Error on accept\n");
		}
		else
		{
			pthread_t newThreadSend;
			pthread_create(&newThreadSend,NULL,sendTheData,&newSocketfd);
			threadsSend.push_back(newThreadSend);
		}
	}
	
	return NULL;
}


vector<string> getDetailsFromPeer(string peer)
{
	vector<string> args;
	string str="";
	for(int i=0;i<peer.length();i++)
	{
		if (peer[i]==' ')
		{
			args.push_back(str);
			str="";
			
		}
		else
		{
			str+=peer[i];
		}
	}
	args.push_back(str);
	return args;
}


vector<string> getIpAndPort(string args)
{
	string temp="";
	vector<string> result;
	for(int i=0;i<args.length();i++)
	{
		if(args[i]!=':')
		{
			temp+=args[i];
		}
		else
		{
			result.push_back(temp);
			temp="";
		}
	}
	result.push_back(temp);
	return result;
}

void* takeCareOfDownload(void* peer)
{
	string peerFromTracker = *static_cast<string*>(peer);

	vector<string> dataFromTracker = getDetailsFromPeer(peerFromTracker);
	vector<string> ip;vector<string> port;

	string destPath =dataFromTracker[dataFromTracker.size()-3];
	string inputFileName=dataFromTracker[dataFromTracker.size()-2];
	string groupid = dataFromTracker[dataFromTracker.size()-1];
	for(int i=1;i<dataFromTracker.size()-3;i++)
	{
		vector<string> ipAndPort = getIpAndPort(dataFromTracker[i]);
		if(ipAndPort[1]==myPort) continue;
		port.push_back(ipAndPort[1]);
		ip.push_back(ipAndPort[0]);
		
	}
	int anothersocketfd;
	vector<string> bitmaps;
	vector<string> chunks;
	char buffer[bufferSize];int n;
	string filename = destPath;

	sendDataToTracker("D "+groupid+" "+filename);
	logIntoFile("[+] Download started");
	for(int i=0;i<ip.size();i++)
	{
		anothersocketfd = socket(AF_INET,SOCK_STREAM,0);
	
		struct sockaddr_in peerAddr;
		string peerToAskIp=ip[i];
		int peerPortNum=atoi(port[i].c_str());
		struct hostent *peerToAsk = gethostbyname(peerToAskIp.c_str());
		if(peerToAsk==NULL)
		{
			logIntoFile("[-] Wrong server info or server down!\n");
			
		}
		bzero((char*)&peerAddr,sizeof(peerAddr));
		peerAddr.sin_family =  AF_INET;

		bcopy((char*)peerToAsk->h_addr,(char*)&peerAddr.sin_addr.s_addr,peerToAsk->h_length);
		peerAddr.sin_port = htons(peerPortNum);
		if(connect(anothersocketfd,(struct sockaddr*)&peerAddr,sizeof(peerAddr)) < 0)
		{
			logIntoFile("[-] Error in connecting to peer!\n");	
			return NULL;
		}

		

		bzero(buffer,bufferSize);
		sprintf(buffer, "%s", ("bitmap "+inputFileName).c_str());
		n = write(anothersocketfd,buffer,bufferSize);
		if(n<0)
		{
			logIntoFile("[-] Error in sending input file name!\n");	
		}	

		//get bitmap
		bzero(buffer,bufferSize);
		n = read(anothersocketfd,buffer,bufferSize);
		if(n<0)
		{
			logIntoFile("[-] Error in receing bitmap!\n");	
		}
		bitmaps.push_back(buffer);
		close(anothersocketfd);
	}

	string newChunk;
	vector<int> selectedBitmaps(bitmaps[0].length());
	vector<string> chunksHash;
	string myBitmap;
	for(int i=0;i<bitmaps[0].length();i++)
	{
		for(int j=0;j<bitmaps.size();j++)
		{
			if(bitmaps[i][j]=='1')
			{
				selectedBitmaps[i]=j; //Choosing chunk based on what chunk oldest seeder have
				break;
			}
		}
		myBitmap+='0';
	}

	for(int i=0;i<selectedBitmaps.size();i++)
	{
		anothersocketfd = socket(AF_INET,SOCK_STREAM,0);
	
		struct sockaddr_in peerAddr;
		string peerToAskIp=ip[selectedBitmaps[i]];
		int peerPortNum=atoi(port[selectedBitmaps[i]].c_str());
		struct hostent *peerToAsk = gethostbyname(peerToAskIp.c_str());

		if(peerToAsk==NULL)
		{
			logIntoFile("[-] Wrong server info or server down!\n");	
			
		}
		bzero((char*)&peerAddr,sizeof(peerAddr));
		peerAddr.sin_family =  AF_INET;

		bcopy((char*)peerToAsk->h_addr,(char*)&peerAddr.sin_addr.s_addr,peerToAsk->h_length);
		peerAddr.sin_port = htons(peerPortNum);
		if(connect(anothersocketfd,(struct sockaddr*)&peerAddr,sizeof(peerAddr)) < 0)
		{
			logIntoFile("[-] Error in connecting to peer!\n");			
			return NULL;
		}

		bzero(buffer,bufferSize);
		if(i!=selectedBitmaps.size()-1)
			sprintf(buffer, "%s", (inputFileName+" "+to_string(i)+" notlast").c_str());
		else
		{
			sprintf(buffer, "%s", (inputFileName+" "+to_string(i)+" last").c_str());
		}
		n = write(anothersocketfd,buffer,bufferSize);
		if(n<0)
		{
			logIntoFile("[-] Error in sending input file name!\n");	
			
		}	

		
		bzero(buffer,bufferSize);
		int readBytes;
		newChunk="";
		FILE* outputFile = fopen((myId+"/Downloads/"+filename).c_str(),"ab");
		FILE* outputFileForUpload = fopen((myId+"/Uploads/"+filename).c_str(),"ab");
		
		int stopFlag=0;
		while(1)
		{
			readBytes = recv(anothersocketfd,buffer,bufferSize,0);
			if(readBytes<=0) break;
			while(readBytes<32678 && i!=(selectedBitmaps.size()-1))
			{ 
				//cant decard old buffer
				newChunk+=buffer;
				fwrite(buffer,readBytes,1,outputFile);
				fwrite(buffer,readBytes,1,outputFileForUpload);
				readBytes = recv(anothersocketfd,buffer,bufferSize,0);
				if(readBytes<=0){
					stopFlag=1;
					break;
				}
			}
			if(stopFlag==1) break;
			newChunk+=buffer;
			// sleep(1);
			fwrite(buffer,readBytes,1,outputFile);
			fwrite(buffer,readBytes,1,outputFileForUpload);
			bzero(buffer,bufferSize);
		}
		bzero(buffer,bufferSize);
		fclose(outputFile);
		fclose(outputFileForUpload);
		close(anothersocketfd);
	}

	string myHash = getShaHash(myId+"/Downloads/"+filename);
	logIntoFile("[+] Download completed");
	sendDataToTracker("C "+groupid+" "+filename+" "+myHash);
	sendDataToTracker("hash upload_file "+filename+" "+groupid+" "+myHash);
	files[filename]=myBitmap;

	return NULL;
}



void* handleCommands(void* sfd)
{
	int socketfd = socket(AF_INET,SOCK_STREAM,0);
	socketfd = *((int*)sfd);
	while(1)
	{
		string command="";
		cout<<"Command:";
		getline(cin,command);
		int commandType = processInputCommand(command);
		string trackerReply = sendDataToTracker(command);
		
		if(commandType==1)
		{
			if(isdigit(trackerReply[0]))
			{
				string threadInput = "TrackerReply "+trackerReply;
				pthread_t newThread;
				pthread_create(&newThread,NULL,takeCareOfDownload,&threadInput);
				threadsRecv.push_back(newThread);
			}
		}
		else if(commandType==2 && trackerReply=="File not available")
		{
			cout<<trackerReply<<endl;
		}
		else if(command=="exit")
		{
			sendDataToTracker("logout");
			exit(0);
			break;
		}
	}
	close(socketfd);
	return NULL;
}

vector<string> getIpPort(char* argv[])
{
	int flag=0;
	vector<string> result;
	string ip="",port="";
	string inputString = argv[1];
	for(int i=0;i<inputString.length();i++)
	{
		if(inputString[i]!=':' && flag==0)
		{
			ip+=inputString[i];
		}else if(inputString[i]==':')
		{
			flag=1;
			continue;
		}
		else if(inputString[i]!=':' && flag==1)
		{
			port+=inputString[i];
		}
	}
	result.push_back(ip);
	result.push_back(port);
	return result;
}

int main(int argc,char* argv[])
{
	if(argc<3)
	{
		logIntoFile("[-] Tracker info or client info not provided\n");
		return 0;
	}
	myIp = getIpPort(argv)[0];
	myPort = getIpPort(argv)[1];

	trackerFileName = argv[2];

	int socketfd,newSocketfd,n,portno;
	char buffer[bufferSize];
	struct sockaddr_in serverAddr,clientAddr;
	socklen_t clientAddrLen;
	socketfd = socket(AF_INET,SOCK_STREAM,0);
	if(socketfd<0) 
	{
		logIntoFile("[-] Error opening socket\n");
		
	}
	bzero((char*)&serverAddr,sizeof(serverAddr));

	portno = atoi(myPort.c_str());
	serverAddr.sin_family =  AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(portno);


	if(bind(socketfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) < 0)
	{
		logIntoFile("Error in binding!\n");
	}

	//this will take care of getting commands
	pthread_t commandThread;
	pthread_create(&commandThread,NULL,handleCommands,&socketfd);
	
	// this will take care of uploading to other
	pthread_t sendingDataThread;
	
	pthread_create(&sendingDataThread,NULL,handleSendingDataThread,&socketfd);

	pthread_join(commandThread,NULL);
	pthread_join(sendingDataThread,NULL);
	


	return 0;
}









