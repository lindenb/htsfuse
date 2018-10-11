/**
small utility to convert 1000 genome tree to htsfuse XML file 

 g++ 1000g.cpp && wget -q  -O - "http://ftp.1000genomes.ebi.ac.uk/vol1/ftp/current.tree" | cut -f 1  |\
   grep -E '\.(bam|\.bai)$' | ./a.out 

*/
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;


class Node {
	public:
		bool file_flag;
		string name;
		map<string,Node*> children;

		void insert(string& url,vector<string>& tokens,size_t i) {
		if(i+1==tokens.size())
			{
			Node *n = new Node;
			n->name = url;
			n->file_flag=true;
			children[tokens[i]]=n;
			return;
			}
		map<string,Node*>::iterator r= children.find(tokens[i]);
		if(r!=children.end())
			{
			r->second->insert(url,tokens,i+1);
			}
		else
			{
			Node *n = new Node;
                        n->name = tokens[i];
                        n->file_flag=false;
                        children[n->name]=n;
			n->insert(url,tokens,i+1);
			}
		}
		
		void dump()
			{
			if(file_flag) {
				cout << "<file url=\"http://ftp.1000genomes.ebi.ac.uk/vol1/" << name << "\"/>" << endl;
				}
			else
				{
				cout << "<directory name=\"" << name << "\">" << endl;
				for( map<string,Node*>::iterator r=children.begin();r!=children.end();++r)
					{
					r->second->dump();
					}
				cout << "</directory>" << endl;
				}
			}
	};

int main(int argc,char** argv)
	{
	string line;
	Node root;
	root.name = "root";
	root.file_flag=false;
	while(getline(cin,line))
		{
		vector<string> tokens;
		size_t i=0,prev=0;
		for(i=0;i< line.size();i++)
			{
			if(line[i]=='/')
				{
				tokens.push_back(line.substr(prev,i-prev));
				prev=i+1;
				}
			}
		tokens.push_back(line.substr(prev));
		root.insert(line,tokens,0);
		}

	root.dump();
	return 0;
	}
