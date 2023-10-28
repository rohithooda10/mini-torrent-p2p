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
#include <unordered_map>
#include <sys/stat.h>
#include <ctype.h>
#include <cmath>

using namespace std;
#define bufferSize 512*100
string logFileName="SystemLogs.txt";
string myTrackerNo="1";
class Group{
public:
	string groupid;
	vector<vector<string> > members;
	vector<vector<string> > pendingRequests;
	vector<vector<string> > files;
	Group(string newgroupid,string userName){
		groupid=newgroupid;
		vector<string> temp;
		temp.push_back(userName);
		temp.push_back("admin");
		members.push_back(temp);
	}
};

unordered_map<string,Group> groups;
vector<vector<string> > allUsers;

string loggedInUser="";
pthread_t threads[100];
int thread_count=0;

class UserData{
public:
	vector<string> downloading;
	vector<string> completed;
};

unordered_map<string,UserData> userDownloads;

void logIntoFile(string logstring)
{
	ofstream fout;
	fout.open(("Tracker_"+myTrackerNo+"_"+logFileName),ios::app);
	fout<<"\n"<<logstring;
}

bool fileExists(string filename)
{
	ifstream infile(filename.c_str());
	return infile.good();
}

vector<string> getArgs(string command)
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
	return args;
}

string createUser(string userid,string password,string ip,string port)
{
	string result="";

	for(int i=0;i<allUsers.size();i++)
	{
		if(allUsers[i][0]==userid)
		{
			result+="User already exist!";
			return result;
		}
	}

	vector<string> newUser;
	newUser.push_back(userid);
	newUser.push_back(password);
	newUser.push_back(ip);
	newUser.push_back(port);
	newUser.push_back("false");
	allUsers.push_back(newUser);

	UserData userData;
	userDownloads.insert(make_pair(userid,userData));

	result="User signed up!";
	return result;
}

bool isUserLoggedIn(string userid)
{
	for(int i=0;i<allUsers.size();i++)
	{
		if(allUsers[i][0]==userid && allUsers[i][4]=="active")
		{
			return true;
		}
	}
	return false;
}

bool isFileThere(string groupid,string filename)
{
	for(int j=0;j<groups.at(groupid).files.size();j++)
	{
		if( groups.at(groupid).files[j][0] == filename )
		{
			return true;
		}
	}
	return false;
}

string getNameFromDetails(string ip,string port)
{
	for(int i=0;i<allUsers.size();i++)
	{
		if(allUsers[i][2]==ip && allUsers[i][3]==port)
		{
			return allUsers[i][0];
		}
	}
	return "DefaultName";
}

string loginUser(string userid,string password,string ip,string port)
{
	string result="";

	if(isUserLoggedIn(userid))
	{
		string result= "User already loggedIn!";
		return result;
	}
	for(int i=0;i<allUsers.size();i++)
	{
		if(allUsers[i][0]==userid && allUsers[i][1]==password)
		{
			allUsers[i][4]="active";
			allUsers[i][2]=ip;
			allUsers[i][3]=port;
			result+="User logged in";
			return result;
		}
	}
	result="User not found! Wrong credentials";
	return result;
}


bool isAdmin(string groupid,string userid)
{
	for(int j=0;j<groups.at(groupid).members.size();j++)
	{
		if(groups.at(groupid).members[j][0]==userid 
			&& groups.at(groupid).members[j][1]=="admin")
		{
			return true;
		}
	}
	return false;
}

bool isMember(string groupid,string userid)
{
	for(int j=0;j<groups.at(groupid).members.size();j++)
	{
		if(groups.at(groupid).members[j][0]==userid)
		{
			return true;
		}
	}
	return false;
}

bool isAlreadyPending(string groupid,string userid)
{
	for(int j=0;j<groups.at(groupid).members.size();j++)
	{
		if(groups.at(groupid).pendingRequests[j][0]==userid)
		{
			return true;
		}
	}
	return false;
}

string logoutUser(string userid)
{
	if(!isUserLoggedIn(userid))
	{
		string result= "Login first";
		return result;
	}
	for(int i=0;i<allUsers.size();i++)
	{
		if(allUsers[i][0]==userid)
		{
			allUsers[i][4]="false";
			break;
		}
	}
	return "User logged out";
}

vector<string> getSeederName(string list)
{
	string str="";
	vector<string> args;
	for(int i=0;i<list.length();i++)
	{
		if (list[i]==' ')
		{
			args.push_back(str);
			str="";
			
		}
		else
		{
			str+=list[i];
		}
	}
	args.push_back(str);
	return args;
}

string uploadFile(string filePath,string groupid,string userid,string numberOfChunks,string hash)
{
	string result="";
	if(!isUserLoggedIn(userid))
	{
		result+= "Login first";
		return result;
	}
	if(groups.find(groupid)==groups.end())
	{
		result+="Group doesn't exist";
		return result;
	}
	if(!isMember(groupid,userid))
	{
		result="Not a member! Join group first";
		return result;
	}
	if(isFileThere(groupid,filePath))
	{
		for(int i=0;i<groups.at(groupid).files.size();i++)
		{
			if(groups.at(groupid).files[i][0]==filePath && hash==groups.at(groupid).files[i][3])
			{
				groups.at(groupid).files[i][1]+= " "+userid; 
			}
		}

		return "File uploaded";
	}

	vector<string> newFile;
	newFile.push_back(filePath);
	newFile.push_back(userid);
	newFile.push_back(numberOfChunks);
	newFile.push_back(hash);

	groups.at(groupid).files.push_back(newFile);
	return "File uploaded";
}

string stopSharing(string filePath,string groupid,string userid)
{
	string result="";
	if(!isUserLoggedIn(userid))
	{
		result+= "Login first";
		return result;
	}
	if(groups.find(groupid)==groups.end())
	{
		result+="Group doesn't exist";
		return result;
	}
	if(!isMember(groupid,userid))
	{
		result="Not a member! Join group first";
		return result;
	}
	if(!isFileThere(groupid,filePath))
	{
		result="File not present in this group!";
		return result;
	}
	int index;
	for(int i=0;i<groups.at(groupid).files.size();i++)
	{
		if(groups.at(groupid).files[i][0]==filePath)
		{
			vector<string> allOwners = getArgs(groups.at(groupid).files[i][1]);
			// cout<<"no of owners:"<<allOwners.size();
			// cout<<userid<<endl;
			if(allOwners.size()==1)
			{
				if(allOwners[0]==userid)
				{
					index=i;
					break;
				}
				else
				{
					result+="You are not sharing this file";
					return result;
				}
			}
			else{
				string newOwners="";
				for(int j=0;j<allOwners.size();j++)
				{
					if(allOwners[j]!=userid && j!=(allOwners.size()-1))
					{
						newOwners+=allOwners[j]+" ";
					}
					else if(allOwners[j]!=userid && j==(allOwners.size()-1))
					{
						newOwners+=allOwners[j];
					}
				}
				if(newOwners!="")
					groups.at(groupid).files[i][1] = newOwners;
				else
				{
					index=i;
					break;
				}
				// cout<<groups.at(groupid).files[i][1]<<endl;
				return "Stopped sharing the file!";
			}
			
		}
	}
	groups.at(groupid).files.erase(groups.at(groupid).files.begin()+index);
	return "Stopped sharing!";
	
}
string createGroup(string groupid,string userid)
{
	string result="";

	//data structure method
	if(!isUserLoggedIn(userid))
	{
		result+= "Login first";
		return result;
	}
	string userName = userid;
	Group newGroup(groupid,userName);
	if(groups.find(groupid)!=groups.end())
	{
		result+="Group already exists";
	}
	else
	{
		groups.insert(make_pair(groupid,newGroup));
		result+="Group added";
	}
	return result;
}

string joinGroup(string groupid,string userid)
{
	string result;
	
	if(!isUserLoggedIn(userid))
	{
		result+= "Login first";
		return result;
	}
	if(groups.find(groupid)==groups.end())
	{
		result+="Group doesn't exist";
	}
	else if(isMember(groupid,userid))
	{
		result+= "Already a member";
	}
	else if(groups.at(groupid).pendingRequests.size()>0 && isAlreadyPending(groupid,userid))
	{
		result+="Request already pending";
	}
	else
	{
		string userName = userid;
		vector<string> temp;
		temp.push_back(userName);
		temp.push_back("member");
		groups.at(groupid).pendingRequests.push_back(temp); //in accept request add member tag
		result+="Request added to the group";
	}
	return result;
}

string listGroups()
{
	string result="\n";
	for(auto i:groups)
	{
		result+= i.first+"\n";
	}
	return result;
}

string showMembers(string groupid)
{
	string result="\n";
	if(groups.find(groupid)==groups.end())
	{
		result+="Group doesn't exist";
	}
	else
	{
		for(int j=0;j<groups.at(groupid).members.size();j++)
		{
			result += (groups.at(groupid).members[j][0]+" "+groups.at(groupid).members[j][1]+"\n");
		}
	}
	return result;
}

string listFiles(string groupid)
{
	string result="\n";
	if(groups.find(groupid)==groups.end())
	{
		result+="Group doesn't exist";
	}
	else
	{
		for(int j=0;j<groups.at(groupid).files.size();j++)
		{
			result += (groups.at(groupid).files[j][0]+"\n");
		}
	}
	return result;
}

string showPendingRequests(string groupid,string userid)
{
	string result="\n";
	if(groups.find(groupid)==groups.end())
	{
		result="Group doesn't exist";
	}
	else if(!isAdmin(groupid,userid))
	{
		result="You don't have permission to access this data!";
	}
	else
	{
		for(int j=0;j<groups.at(groupid).pendingRequests.size();j++)
		{
			result += (groups.at(groupid).pendingRequests[j][0]+"\n");
		}
	}
	return result;
}

string acceptRequest(string groupid,string userid,string userName)
{
	string result="";
	if(groups.find(groupid)==groups.end())
	{
		result="Group doesn't exist";
	}
	else if(!isAdmin(groupid,userid))
	{
		result="You don't have permission to accept this request!";
	}
	else
	{
		for(int j=0;j<groups.at(groupid).pendingRequests.size();j++)
		{
			if( groups.at(groupid).pendingRequests[j][0] == userName )
			{
				groups.at(groupid).members.push_back(groups.at(groupid).pendingRequests[j]);
				groups.at(groupid).pendingRequests.erase(groups.at(groupid).pendingRequests.begin()+j);
				result+="Request accepted";
				break;
			}
		}
	}
	return result;
}

string showAllUser()
{
	for(int i=0;i<allUsers.size();i++)
	{
		cout<<allUsers[i][0]<<" "<<allUsers[i][1]<<" "<<allUsers[i][2]<<" "<<allUsers[i][3]<<" "<<allUsers[i][4]<<endl;
		
	}
	return "allMembers";
}

string showDownloads(string userName)
{
	string result="\n";
	if(userDownloads.find(userName)==userDownloads.end())
	{
		result+="[No downloads]";
		return result;
	}
	UserData userData = userDownloads.at(userName);
	if(userData.downloading.size() > 0)
	{
		for(int i=0;i<userData.downloading.size();i++)
		{
			result+= "[D]"+userData.downloading[i]+"\n";
		}
	}
	if(userData.completed.size() > 0)
	{
		for(int i=0;i<userData.completed.size();i++)
		{
			result+= "[C]"+userData.completed[i]+"\n";
		}
	}
	if(userData.completed.size()==0 && userData.downloading.size()==0) result="[No downloads]";
	return result;
	
}

string download(string filename,string destPath,string userid,string groupid)
{
	string result="";
	if(!isUserLoggedIn(userid))
	{
		result+= "Login first";
		return result;
	}
	if(groups.find(groupid)==groups.end())
	{
		result+="Group doesn't exist";
		return result;
	}
	if(!isFileThere(groupid,filename))
	{
		result="File not present in this group!";
		return result;
	}
	if(!isMember(groupid,userid))
	{
		result="Not a member! Join group first";
		return result;
	}
	for(int i=0;i<groups.at(groupid).files.size();i++)
	{
		if(groups.at(groupid).files[i][0]==filename)
		{
			vector<string> seedername = getSeederName(groups.at(groupid).files[i][1]);
			for(int k=0;k<seedername.size();k++)
			{
				for(int j=0;j<allUsers.size();j++)
				{
					if(allUsers[j][0]==seedername[k] && isUserLoggedIn(seedername[k]) && seedername[k]!=userid)
					{
						result+= allUsers[j][2]+":";
						result+= allUsers[j][3]+" ";
						
					}
				}
			}
		}
	}
	result+=destPath+" ";
	result+=filename+" ";
	result+=groupid;
	return result;
}

string leaveGroup(string groupid,string userid)
{
	string result="";
	if(groups.find(groupid)==groups.end())
	{
		result+="Group doesn't exist";
		return result;
	}
	if(!isMember(groupid,userid))
	{
		result="Not a member! Join group first";
		return result;
	}
	int flag=0;
	for(int j=0;j<groups.at(groupid).members.size();j++)
	{
		if( groups.at(groupid).members[j][0] == userid)
		{
			if(groups.at(groupid).members[j][1] == "admin") flag=1;
			groups.at(groupid).members.erase(groups.at(groupid).members.begin()+j);
			
			break;
		}
	}
	//delete files of left user
	vector<vector<string> > temp;
	for(int i=0;i<groups.at(groupid).files.size();i++)
	{
		vector<string> allOwners = getArgs(groups.at(groupid).files[i][1]);
		int flag=0;
		if(allOwners.size()==1 && allOwners[0]==userid)
			flag=1;
		else
		{
			string newOwners="";
			for(int j=0;j<allOwners.size();j++)
			{
				if(allOwners[j]!=userid && j!=(allOwners.size()-1))
				{
					newOwners+=allOwners[j]+" ";
				}
				else if(allOwners[j]!=userid && j==(allOwners.size()-1))
				{
					newOwners+=allOwners[j];
				}
			}
			groups.at(groupid).files[i][1]=newOwners;
		}
		if(flag==1) continue;
		else
		{
			temp.push_back(groups.at(groupid).files[i]);
		}
		
	}
	groups.at(groupid).files = temp;
	
	//if no member left delete group
	if(groups.at(groupid).members.size()<=0)
	{
    	groups.erase(groups.find(groupid));

	}
	else if(flag==1) //if it was admin find a new one
	{
		for(int j=0;j<groups.at(groupid).members.size();j++)
		{
			if( groups.at(groupid).members[j][1] == "member")
			{
				groups.at(groupid).members[j][1] = "admin";
				break;
			}
		}
	}
    result="Left the group!";
    return result;
    
}

void addDownloading(string groupid,string userid,string filename)
{
	// cout<<"USERID:"<<userid<<"ENTERED:"<<("["+groupid+"]"+filename)<<endl;
	userDownloads.at(userid).downloading.push_back(("["+groupid+"]"+filename));
}
void addCompleted(string groupid,string userid,string filename,string hash)
{
	// cout<<"USER:"<<userid<<"-";
	int index;
	for(int i=0;i<userDownloads.at(userid).downloading.size();i++)
	{
		// cout<<userDownloads.at(userid).downloading[i]<<" ";
		if(userDownloads.at(userid).downloading[i]==("["+groupid+"]"+filename))
		{
			index=i;
			break;
		}
	}
	// cout<<endl;
	// cout<<"done";
	
	userDownloads.at(userid).downloading.erase(userDownloads.at(userid).downloading.begin()+index);
	
	string msgtoput;
	for(int i=0;i<groups.at(groupid).files.size();i++)
	{
		if(groups.at(groupid).files[i][0]==filename)
		{
			if(hash==groups.at(groupid).files[i][3])
			{
				msgtoput="[File verified]";
				break;
			}
			else 
			{
				msgtoput="[File may be corrupted! Try downloading again.]";
				break;
			}
		}
	}

	
	userDownloads.at(userid).completed.push_back("["+groupid+"]"+filename+" "+ msgtoput);

}

string processInputCommand(vector<string> args)
{
	string result;
	
	if(args[0]=="create_user")
	{
		string userid,password;
		userid=args[1];
		password=args[2];
		string ip=args[4];
		string port=args[5];
		logIntoFile("[i] Client requesting sign up: "+userid);
		result = createUser(userid,password,ip,port);
	}
	else if(args[0]=="login")
	{
		string userid,password;
		userid=args[1];
		password=args[2];
		string ip=args[4];
		string port=args[5];
		logIntoFile("[i] Client requesting login: "+userid);
		result = loginUser(userid,password,ip,port);
	}
	else if(args[0]=="show_downloads")
	{
		string userid=args[1];
		logIntoFile("[i] Client requesting to show downloads: "+userid);
		result = showDownloads(userid);
	}
	else if(args[0]=="show_users")
	{
		result=showAllUser();
	}
	else if(args[0]=="list_requests")
	{
		string userid=args[2];
		logIntoFile("[i] Client requesting to see requests: "+userid);
		result = showPendingRequests(args[1],userid);
	}
	else if(args[0]=="logout")
	{
		string userid=args[1];
		logIntoFile("[i] Client requesting logout: "+userid);
		result = logoutUser(userid);
	}
	else if(args[0]=="create_group") 
	{
		string groupid = args[1];
		string userid=args[2];
		logIntoFile("[i] Client requesting to create group: "+userid);
		result = createGroup(groupid,userid);
	}
	else if(args[0]=="leave_group")
	{
		string groupid = args[1];
		string userid=args[2];
		logIntoFile("[i] "+userid+" requesting to leave group: "+groupid);
		result = leaveGroup(groupid,userid);
	}
	else if(args[0]=="join_group")
	{
		string groupid = args[1];
		string userid=args[2];
		logIntoFile("[i] "+userid+" requesting to join group: "+groupid);
		result = joinGroup(groupid,userid);
	}
	else if(args[0]=="accept_request")
	{
		string groupid = args[1];
		string userName = args[2];
		string userid=args[3];
		logIntoFile("[i] "+userid+" requesting to accepting request");
		result = acceptRequest(groupid,userid,userName);
	}
	else if(args[0]=="upload_file")
	{
		string filePath=args[1];
		string groupid=args[2];
		string userid=args[3];
		string numberOfChunks = args[6];
		string hash = args[7];
		logIntoFile("[i] "+userid+" requesting to upload file to: "+groupid);
		result = uploadFile(filePath,groupid,userid,numberOfChunks,hash);
	}
	else if(args[0]=="stop_share")
	{
		string filePath=args[2];
		string groupid=args[1];
		string userid=args[3];
		logIntoFile("[i] "+userid+" requesting to stop share for file: "+filePath);
		result = stopSharing(filePath,groupid,userid);
	}
	else if(args[0]=="list_groups")
	{
		logIntoFile("[i] Requesting group list");
		result = listGroups();
	}
	else if(args[0]=="show_members")
	{
		result = showMembers(args[1]);
	}
	else if(args[0]=="list_files")
	{
		string groupid = args[1];
		logIntoFile("[i] Requesting to see files in group: "+groupid);
		result = listFiles(groupid);
	}
	else if(args[0]=="D")
	{
		string groupid = args[1];
		string filename = args[2];
		string userid=args[3];
		logIntoFile("[i] "+userid+" started download for: "+filename);
		addDownloading(groupid,userid,filename);
	}
	else if(args[0]=="C")
	{
		string groupid = args[1];
		string filename = args[2];
		string userid=args[4];
		string hash = args[3];
		logIntoFile("[i] "+userid+" acknowledging completion of download of file "+filename);
		addCompleted(groupid,userid,filename,hash);
	}
	else if(args[0]=="download")
	{
		string groupid=args[1];
		string filename=args[2];
		string destPath=args[3];
		string userid=args[4];
		logIntoFile("[i] "+userid+" requesting to download file: "+filename);
		result = download(filename,destPath,userid,groupid);
	}else{
		result="Invalid command!";
	}
	return result;
}

void* handleCommunication(void* sfd)
{
	int newSocketfd = *((int*)sfd);

	int n;
	char buffer[bufferSize];
	bzero(buffer,bufferSize);
	n = read(newSocketfd,buffer,bufferSize);
	if(n<0)
	{
		logIntoFile("[-] Error reading request from client");
		
	}
	
	string command = buffer;
	
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
	string result  = processInputCommand(args);
	bzero(buffer,bufferSize);
	
	sprintf(buffer, "%s", result.c_str());
	n = write(newSocketfd,buffer,bufferSize);
	if(n<0)
	{
		logIntoFile("[-] Error replying to client\n");
		
	}
	bzero(buffer,bufferSize);
	close(newSocketfd);
	return NULL;
}

vector<string> getTrackerInfo(char* argv[])
{
	vector<string> result;
	string num,ip,port;
	string trackerno=argv[2];
	ifstream trackerFile;
	trackerFile.open(argv[1]);
	while(trackerFile >> num >> ip >> port)
	{
		if(num==trackerno)
		{
			result.push_back(ip);
			result.push_back(port);
			break;
		}
	}

	return result;
}

void* handleCommands(void* sfd)
{
	int socketfd = socket(AF_INET,SOCK_STREAM,0);
	socketfd = *((int*)sfd);
	while(1)
	{
		string command="";
		getline(cin,command);
		
		if(command=="quit")
		{
			cout<<"================= Tracker offline =================\n";
			logIntoFile("[i] Tracker closing!\n");
			exit(0);
			break;
		}
	}
	close(socketfd);
	return NULL;
}

void* handleEverythingElseThread(void* sfd)
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen;
	int socketfd = socket(AF_INET,SOCK_STREAM,0);
	socketfd = *((int*)sfd);
	while(1)
	{
		//make threads here so tracekr can handle many

		int newSocketfd = accept(socketfd,(struct sockaddr*)&clientAddr,&clientAddrLen);
		if(newSocketfd<0) 
		{
			logIntoFile("[-] Error on accept\n");
		}
		else
		{
			pthread_create(&threads[thread_count++],NULL,handleCommunication,&newSocketfd);
			
		}
		// break;
	}
}

int main(int argc,char* argv[])
{
	if(argc<3)
	{
		logIntoFile("[-] Tracker info not provided\n");
		return 0;
	}
	vector<string> trackerInfo = getTrackerInfo(argv);
	myTrackerNo=argv[2];
	//read from file if exists and perform processInput
	if(fileExists("commands.txt"))
	{
		logIntoFile("[i] Syncing tracker..");
		ifstream commandFile("commands.txt");
		string temp;
	    while (getline(commandFile, temp)) {
	        vector<string> args;
			string str="";
			for(int i=0;i<temp.length();i++)
			{
				if (temp[i]==' ')
				{
					args.push_back(str);
					str="";
					
				}
				else
				{
					str+=temp[i];
				}
			}
			args.push_back(str);
			string result  = processInputCommand(args);
	    }
	}
	cout<<"================= Tracker online =================\n";
	logIntoFile("[+] Tracker online and synced up!\n");
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

	portno = atoi(trackerInfo[1].c_str());
	
	serverAddr.sin_family =  AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(portno);


	if(bind(socketfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) < 0)
	{
		logIntoFile("[-] Error in binding!\n");
	}

	listen(socketfd,100);
	clientAddrLen = sizeof(clientAddr);

	//this will take care of getting commands
	pthread_t commandThread;
	pthread_create(&commandThread,NULL,handleCommands,&socketfd);
	
	// this will take care of everything else
	pthread_t everythingElse;
	pthread_create(&everythingElse,NULL,handleEverythingElseThread,&socketfd);

	pthread_join(commandThread,NULL);
	pthread_join(everythingElse,NULL);
	
	

	close(socketfd);

	return 0;
}









