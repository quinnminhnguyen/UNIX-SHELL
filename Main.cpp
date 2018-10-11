#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <sys/types.h>
#include <math.h>
#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <cstring>
#include <sys/wait.h>
#include <fstream>
#include <vector>
#include <fcntl.h>
#include <stdlib.h>
using namespace std;

#define BUFFER_SIZE 256
vector<pid_t> bg_process;

struct Tokenizer{
	vector<string> tokens;
	vector<Tokenizer> cmdTokens;
	int isPipe;
	bool isIORedirect;
	bool isBackground;

	Tokenizer(vector<string> tokens, bool isIORedirect, bool isBackground){
		this->tokens = tokens;
		this->isIORedirect = isIORedirect;
		this->isBackground = isBackground;
	}
	Tokenizer(string command){
		isPipe = 0;
		isIORedirect = false;
		isBackground = false;
		string aToken = "";

		for(int i =0; i < command.size(); i++){
			switch(command.at(i)){
				case '\'':
				case '"':
					i++;
					do{
						aToken.push_back(command.at(i));
						i++;
					}while(command.at(i) != '"' && command.at(i) != '\'');
					tokens.push_back(aToken);
					aToken = "";
					break;
				case '|':
				{
					if(aToken.size() != 0){
						tokens.push_back(aToken);
						aToken = "";
					}
					isPipe++;
					Tokenizer tempToken(tokens, isIORedirect, isBackground);
					cmdTokens.push_back(tempToken);
					isIORedirect = false;
					//isBackground = false;
					while(tokens.size() != 0)
						tokens.pop_back();
					break;
				}
				case ' ':
					if(aToken.size() > 0)
						tokens.push_back(aToken);
					aToken = "";
					break;
				case '&':
					if(aToken.size() != 0){
						tokens.push_back(aToken);
						aToken = "";
					}
					isBackground = true;
					//tokens.push_back("&");
					break;
				case '>':
				case '<':
				{
					if(aToken.size() != 0){
						tokens.push_back(aToken);
						aToken = "";
					}


					isIORedirect = true;
					string ioRedirect = "";
					ioRedirect.push_back(command.at(i));
					tokens.push_back(ioRedirect);
					break;
				}
				default:
					aToken.push_back(command.at(i));
					if(i == (command.size() -1)){
						tokens.push_back(aToken);
					}
					break;
			}
		}
		if(tokens.size() > 0){
			Tokenizer tempToken(tokens,isIORedirect, isBackground);
			cmdTokens.push_back(tempToken);
			isIORedirect = false;
			//isBackground = false;
			while(tokens.size() != 0)
				tokens.pop_back();
		}
		
		//Debug purpose
		//cout << "token size = " << tokens.size() <<endl;
		/*
		for(int i = 0; i < cmdTokens.size(); i++){
			for(int j = 0; j < cmdTokens.at(i).size(); j++){
				cout << cmdTokens.at(i).at(j) << " ";
			}
			cout << endl;
		}
		*/
		/*
		cout << endl;
		cout << "isPipe = " << isPipe <<endl;
		cout << "isIORedirect = " << isIORedirect <<endl;
		cout << "isBackground = " << isBackground <<endl;
		*/
	}
	
	void printAllTokens(){
		cout << "total commands = " << cmdTokens.size() <<endl;
		for(int i = 0; i  < cmdTokens.size(); i++){
			cout << "curr tokens size = " << cmdTokens.at(i).tokens.size() <<endl;
			for(int j =0; j < cmdTokens.at(i).tokens.size(); j++){
				cout << cmdTokens.at(i).tokens.at(j) << " ";
			}
			cout << endl;
			cout << "isIORedirect = " << cmdTokens.at(i).isIORedirect <<endl;
			cout << "isBackground = " << cmdTokens.at(i).isBackground <<endl;
		}
		return;
	}
	
};



int shellExec(Tokenizer allTokens){

	pid_t  pidMain = fork();
	if(pidMain == 0){
		//cout << "cmdTokens size = " << allTokens.cmdTokens.size() <<endl;
		for(int k = 0; k < allTokens.cmdTokens.size(); k++){
			//cout << "first for loop" << endl;
			//vector<string> currtokens = allTokens.cmdTokens.at(k);
			char * argV[allTokens.cmdTokens.at(k).tokens.size()+1];
			int indexOfArg = 0;

			int fd = 0;
			int outIn = 0;

			for(int i = 0; i <  allTokens.cmdTokens.at(k).tokens.size(); i++){
				if(allTokens.cmdTokens.at(k).tokens.at(i) == "cd"){
					int nexti = i +1;
					char *temp = new char[allTokens.cmdTokens.at(k).tokens.size()];
					if((i == allTokens.cmdTokens.at(k).tokens.size() -1) || allTokens.cmdTokens.at(k).tokens.at(nexti) == "~"){
						int ret = chdir(getenv("HOME"));
						return 0;
					}

					if(allTokens.cmdTokens.at(k).tokens.at(nexti) == "-"){
						allTokens.cmdTokens.at(k).tokens.at(nexti) = "..";
					}

					strcpy(temp, allTokens.cmdTokens.at(k).tokens.at(nexti).c_str());


					int ret = chdir(temp);
					return  0;
				}

				//for IO redirection
				if(allTokens.cmdTokens.at(k).tokens.at(i) == ">" || allTokens.cmdTokens.at(k).tokens.at(i) == "<"){
					//int nextIndex = i++;
					i++;
					//cout << "file name is " << allTokens.tokens.at(i) <<endl;
					char *fileName = new char[allTokens.cmdTokens.at(k).tokens.size()];
					strcpy(fileName,allTokens.cmdTokens.at(k).tokens.at(i).c_str());

					i--;

					if(allTokens.cmdTokens.at(k).tokens.at(i) == "<"){
						fd = open(fileName, O_RDONLY,0);
						outIn = STDIN_FILENO;
					}
					else{
						fd = open(fileName, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
						outIn = STDOUT_FILENO;
					}

					//cout << "outIn = " << outIn <<endl;
					i++; 

					break;
				}

				char *temp = new char[allTokens.cmdTokens.at(k).tokens.size()];

				strcpy(temp, allTokens.cmdTokens.at(k).tokens.at(i).c_str());

				argV[indexOfArg] = temp;
				//cout << "curr index = " << indexOfArg <<endl;
				//cout << "curr token = " << argV[indexOfArg] <<endl;
				indexOfArg++;
			}
			//cout << "indexOfArg = " <<indexOfArg <<endl;
			argV[indexOfArg] = NULL;

			if(k == allTokens.cmdTokens.size() -1){
				if(allTokens.cmdTokens.at(k).isIORedirect){
					dup2(fd, outIn);
					close(fd);
				}
				execvp(argV[0], argV);
				break;
			}

			int pipefd[2];
			pipe(pipefd);

			pid_t pid = fork();

			if(pid == 0){
				//cout << "Hello from child" <<endl;
				dup2(pipefd[1],1);
				close(pipefd[0]);
				if(allTokens.cmdTokens.at(k).isIORedirect){
					dup2(fd, outIn);
					close(fd);
				}
				int status = execvp(argV[0], argV);
				//cout << "status = " << status <<endl; 
			}
			else{
				wait(NULL);
				close(pipefd[1]);
				dup2(pipefd[0],0);
				//cout <<"Hello from parent" <<endl;
			}
		}

	}
	else{
		//cout << "curr back ground = " << allTokens.isBackground<<endl;
		if(!allTokens.isBackground)
			wait(NULL);
		else{
			bg_process.push_back(pidMain);
		}
		//cout << "last parent" <<endl;
	}
	return 0;

}


int main(){
	
	while(cin){

		int status;
		for(int i =0; i < bg_process.size(); i++){

			int endID = waitpid(bg_process.at(i), &status, WNOHANG|WUNTRACED);

			if(endID > 0){
				cout << "endid = " << endID <<endl;
				//cout << "killing background " <<endl;
				bg_process.erase(bg_process.begin() + i);
			}
		}
		vector<string> shellCommands;
		string wholeCommand;
		cout <<"SHELL->";
		getline(cin, wholeCommand);
		//cout << "whole command = " << wholeCommand <<endl;

		Tokenizer myToken(wholeCommand);
		//myToken.printAllTokens();

		shellExec(myToken);
	}
	
	return 0;
}