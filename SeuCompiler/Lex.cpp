#include<iostream>
#include<fstream>
#include<string>
#include<unordered_map>
#include<stack>
#include<vector>
#include<list>
#include<set>
#include<queue>
#include<map>
#define LAYER_ID 15//标识%%
#define HEADER_BEGIN 16//标识%{%}
#define HEADER_END 17//
#define BEGIN 0
#define ERROR -11
#define EPSLONG -1
using namespace std;
//定义结点结构
struct Node
{
	unsigned int value;
	unsigned int state;
};
//定义一组常量
ifstream ifile;
ofstream ofile;
int lineno = 0;
vector<list<Node> > nfa;
vector<list<Node> > dfa;
unordered_map<string, string> id2reTable;//存储定义段中标识名到正则式的映射
unordered_map<int, int> nfaTer2Action;//存储NFA终态到action表头对应内容。
unordered_map<int, int> dfaTer2Action;//存储DFA终态到action表头对应内容，其中内容在Nfa2Dfa()时填充
vector<string> actionTable;//存储action内对应内容
vector<int> nfaIsTer;
map< set<int>, int > dfanodetable;
vector<int> dfaIsTer;//标记dfa中哪些是终结态。


//定义一些函数
int checkSpecsign(char c);
void compleRe(string& re);
void produceNfa(const string& re, vector<list<Node> >& tnfa, vector<int>& isTer, int index);
void modifyTer(vector<int>& is_t, unsigned int state, int value);
void joinNfa(vector<list<Node> >& nfa1, const vector<list<Node> >& nfa2);
void joinIster(vector<int>& is_t1, const vector<int>& is_t2);
void Nfa2Dfa();
void Eclosure(set<int>& T);
set<int> move(const set<int>& I, int value);
int dfaIsTerminated(set<int>& I);//返回终态对应动作，如果是非终态返回-1
void minidfa();
void genAnalysisCode();//生成词法分析部分的代码
bool cmpList(const list<Node>& l1, const list<Node>& l2);

void main()
{
	string s;
	cout << "Please input the inputfile name!" << endl;
	cin >> s;
	ifile.open(s.c_str(), ios::in);
	cout << "Analysing source......" << endl;
	ofile.open("yylex.cpp", ios::out);
	if (!ifile.good())
	{
		cerr << "Open the file error!" << endl;
		return;
	}
	char c = ifile.get();
	int state = checkSpecsign(c);
	//判断开头是不是%{
	if (state != HEADER_BEGIN)
	{
		cout << "The input file has no correct formation,Please try again!\n";
		return;
	}
	//判断到%}或到文件尾为止进行扫描
	while (!ifile.eof() && state != HEADER_END)
	{
		c = ifile.get();
		if (c == '\t') continue;//跳过\t字符不输出。
		if (c == '%') { state = checkSpecsign(c); continue; }//当接受到%时，注意判断是不是特殊符号
		if (c == '\n') lineno++;//让行号自增，用以判断错误行号。
		ofile.put(c);
	}
	//以上完成定义段中%{ %}之间的扫描
	//以下开始对定义好的关键字的正规表达式的扫描，并将其存储到表中
	ifile.get();
	pair<string, string> pi;
	state = BEGIN;

	while (!ifile.eof() && state != LAYER_ID)//此处的处理有问题，需要做进一步调整。
	{
		c = ifile.get();
		if (c == '%')
		{
			state = checkSpecsign(c);
			if (state == ERROR)//用户自定义的标识符不可以含有此特殊字符%。
			{
				cerr << "There is an error in line " << lineno << " !" << endl;
				return;
			}
			continue;//跳过下面的正常串的处理，因为已经到终结。
		}
		else
		{
			ifile.unget();//如果不是特殊字符%，把当前字符放回流中。
		}
		string id, re;
		ifile >> id >> re;
		pi.first = id;
		pi.second = re;
		id2reTable.insert(pi);
		lineno++;
		ifile.get();
	}
	//以上是对定义段的扫描
	//以下是对规则段的扫描解析
	state = BEGIN;
	actionTable.push_back("begin");//使action表从1号单元开始，便于处理。
	ifile.get();
	while (!ifile.eof() && state != LAYER_ID)
	{
		c = ifile.get();
		if (c == '%')
		{
			state = checkSpecsign(c);
			if (state == ERROR)//用户自定义的标识符不可以含有此特殊字符%。
			{
				cerr << "There is an error in line " << lineno << " !" << endl;
				return;
			}
			continue;//跳过下面的正常串的处理，因为已经到终结。
		}
		else
		{
			ifile.unget();//如果不是特殊字符%，把当前字符放回流中。
		}
		string onestr;
		string re, action;
		//假设一个动作只能写在一行内。
		getline(ifile, onestr);
		string delim = " \t";
		size_t offset = onestr.find_first_of(delim);
		re = onestr.substr(0, offset);
		while (onestr[offset] == ' ' || onestr[offset] == '\t') offset++;
		action = onestr.substr(offset, onestr.size() - offset);
		actionTable.push_back(action);
		//下面开始正则表达式的扫描替换,即把当中含有用户自定义标识符的地方替换成完整的正则式
		compleRe(re);
		//替换完毕，下面开始依据正则式构造NFA
		vector<list<Node> > nfa1;
		vector<int> isTer;
		int actionTableIndex = actionTable.size() - 1;
		produceNfa(re, nfa1, isTer, actionTableIndex);//产生nfa，及相应的终态表
		joinNfa(nfa, nfa1);//合并nfa
		unsigned int adjustp = nfaIsTer.size();
		joinIster(nfaIsTer, isTer);//合并状态表
		//保存action的内容
		for (unsigned int i = adjustp; i < nfaIsTer.size(); i++)
		{
			if (nfaIsTer[i])
			{
				pair<int, int> p;
				p.first = i;
				p.second = actionTable.size() - 1;
				nfaTer2Action.insert(p);
			}
		}
	}
	//到此一个大的NFA完成了，存储在nfa中。
	Nfa2Dfa();//生成了dfa，放在dfa这个vector中。
	minidfa();//在这个函数中完成了对dfaTer2Action表的修改。
	cout << "Generating code......" << endl;
	genAnalysisCode();
	//下面开始是最后一段，即用户自定义子例程段的输出。
	c = 1;
	while ((c = ifile.get()) != -1)
	{
		ofile.put(c);
	}
	ifile.close();
	ofile.close();
	cout << "Done!" << endl;
}

int checkSpecsign(char c)
{
	if (c == '%')
	{
		char cc = ifile.get();
		switch (cc)
		{
		case '%':
			return LAYER_ID;
		case '{':
			return HEADER_BEGIN;
		case '}':
			return HEADER_END;
		default:
			ifile.unget();
			break;
		}
	}
	return ERROR;
}

void compleRe(string& re)
{
	//此处应当注意一个问题，即如何避免死循环的出现
	//当用户自定义的标识符出现循环嵌套定义时，如何进行识别处理或报错。
	stack<int> status;//状态栈
	status.push(-1);
	unsigned int i = 0;
	while (i < re.size())
	{
		if (re[i] == '{')
			status.push(i);//记录下此时的位置
		if (re[i] == '}')
		{
			int prei = status.top();
			if (prei < 0)
			{
				i++;
				continue;//表示此时的}并无配对，因此直接输出
			}
			int length = i - prei - 1;
			string id = re.substr(prei + 1, length);
			string replacestr = id2reTable[id];
			if (replacestr.empty()) continue;//表示其中的内容不是标识符，忽略不处理
			re.replace(prei, length + 2, replacestr);//+2表示包括{}一起处理
			i = prei - 1;
		}
		i++;
	}
	//处理完毕，此时该正规式中应当没有用户自定义的标识符了
}

void produceNfa(const string& re, vector<list<Node> >& tnfa, vector<int>& isTer, int index)
{
	stack<Node> status;//状态栈，用于存储当前状态
	stack<int> tstatus;
	bool is_reduce = false;//判断是不是已经归约了。
	bool isNonSpec = false;
	unsigned int next_state = 0;
	Node a;
	a.state = -1;
	a.value = -1;
	status.push(a);
	unsigned int i = 0;
	list<Node> p;
	//tnfa.push_back(p);//初始时，先往tnfa内放入一个list<Node>
	while (i < re.size())
	{
		char c = re[i];
		if (c == '}')
		{

		}
		switch (c)
		{
		case '\\':
		{
			isNonSpec = true;
			break;
		}
		case '[':
		{
			if (isNonSpec)
			{
				++next_state;
				a.state = next_state;
				a.value = re[i];
				list<Node> p;
				p.push_back(a);
				tnfa.push_back(p);
				isNonSpec = false;
				break;
			}
			tstatus.push(next_state);
			a.state = re[i];//用state表示状态字符
			a.value = i;//用value表示当时的索引值
			status.push(a);
			is_reduce = false;
			break;
		}
		case '(':
		{
			if (isNonSpec)
			{
				++next_state;
				a.state = next_state;
				a.value = re[i];
				list<Node> p;
				p.push_back(a);
				tnfa.push_back(p);
				isNonSpec = false;
				break;
			}
			a.state = re[i];
			a.value = tnfa.size() - 1;//使value内存储当前nfa中的最高状态值。
			status.push(a);
			break;
		}
		case '*':
		{
			if (isNonSpec)
			{
				++next_state;
				a.state = next_state;
				a.value = re[i];
				list<Node> p;
				p.push_back(a);
				tnfa.push_back(p);
				isNonSpec = false;
				break;
			}
			a = status.top();
			while (a.state == '|')//如果栈内是｜,进行最终态的调整
			{
				int prestate = a.value;
				prestate--;//此时的值表示将要修改的状态点。
				for (list<Node>::iterator p = tnfa[prestate].begin(); p != tnfa[prestate].end(); p++)
				{
					if (p->value != EPSLONG && p->state == prestate + 1)
					{
						p->state = next_state;
					}
				}
				status.pop();
				a = status.top();
			}
			if (a.state == '[')
			{
				int b = tstatus.top();
				a.state = b;
				a.value = EPSLONG;

				list<Node> p;
				tnfa.push_back(p);
				tnfa.back().push_back(a);

				tstatus.pop();
				status.pop();
				if (i == re.size() - 1)
					modifyTer(isTer, b, index);
			}
			else if (a.state == '(')
			{
				int prei = a.value;//取得的是当时nfa中的状态值。
				a.state = prei;
				a.value = EPSLONG;
				list<Node> p;
				tnfa.push_back(p);
				tnfa.back().push_back(a);
				status.pop();
				if (i == re.size() - 1)
					modifyTer(isTer, prei, index);
			}
			break;

		}
		case '|':
		{
			if (isNonSpec)
			{
				++next_state;
				a.state = next_state;
				a.value = re[i];
				list<Node> p;
				p.push_back(a);
				tnfa.push_back(p);
				isNonSpec = false;
				break;
			}
			a = status.top();
			int prestate = 0;
			if (a.state == '|' || a.state == '(')
			{
				prestate = a.value;
			}
			else if (a.state == '[')
			{
				prestate = tstatus.top();
			}
			a.state = next_state;//向nfa内存储一条空边，为了方便进行构造。
			a.value = EPSLONG;

			tnfa[prestate].push_back(a);
			a.state = '|';
			a.value = next_state;//存储此时｜符号前一状态的值
			status.push(a);
			break;
		}
		case ')':
		{
			if (isNonSpec)
			{
				++next_state;
				a.state = next_state;
				a.value = re[i];
				list<Node> p;
				p.push_back(a);
				tnfa.push_back(p);
				isNonSpec = false;
				break;
			}
			a = status.top();
			while (a.state == '|')//如果栈内是｜,进行最终态的调整
			{
				int prestate = a.value;
				prestate--;//此时的值表示将要修改的状态点。
				for (list<Node>::iterator p = tnfa[prestate].begin(); p != tnfa[prestate].end(); p++)
				{
					if (p->value != EPSLONG && p->state == prestate + 1)
					{
						p->state = next_state;
					}
				}
				status.pop();
				a = status.top();
			}
			if (i == re.size() - 1)
				modifyTer(isTer, next_state, index);
			if (i < re.size() - 1 && re[i + 1] != '*') status.pop();
			break;
		}
		case ']':
		{
			if (isNonSpec)
			{
				++next_state;
				a.state = next_state;
				a.value = re[i];
				list<Node> p;
				p.push_back(a);
				tnfa.push_back(p);
				isNonSpec = false;
				break;
			}
			a = status.top();
			if (is_reduce)
			{
				if (re[i + 1] != '*')
				{
					list<Node> p;
					tnfa.push_back(p);
					status.pop();
					tstatus.pop();
				}
				break;
			}
			if (a.state == '[')
			{
				i = a.value;//在后面会再增1。
				is_reduce = true;//设置为已经归约，开始处理
				++next_state;//更新下一状态值
			}
			else if (a.state == -1)
			{//基本上用不到
				++next_state;
				list<Node> p;
				a.value = re[i];
				a.state = next_state;
				p.push_back(a);
				tnfa.push_back(p);
				modifyTer(isTer, next_state, 0);
			}//其余情况什么都不用做。
			break;
		}
		case '-':
		{
			a = status.top();
			list<Node> p;
			if (i >= 1 && i < re.size() - 1 && re[i - 1] < re[i + 1])
			{
				if (a.state != '[')//如果状态栈为空，表示在最外层，可以直接生成
				{
					for (char j = re[i - 1]; j < re[i + 1]; j++)
					{
						++next_state;
						a.state = next_state;
						a.value = j;
						p.push_back(a);
						tnfa.push_back(p);//每次都新增一个状态。
					}
				}
				else
				{
					if (is_reduce)
					{
						for (char j = re[i - 1] + 1; j < re[i + 1]; j++)//re[i-1]+1是因为re[i]已经做过了
						{
							a.state = next_state;
							a.value = j;
							tnfa.back().push_back(a);
						}
					}
				}
			}
			else if (a.state != '[')
			{
				++next_state;
				a.state = next_state;
				a.value = re[i];
				p.push_back(a);
				tnfa.push_back(p);
			}
			break;
		}
		default:
		{
			a = status.top();
			if (a.state == '[')
			{
				if (!is_reduce) break;
				a.state = next_state;
				a.value = re[i];
				if (tnfa.size() == 0)
				{
					list<Node> p;
					tnfa.push_back(p);
				}
				tnfa.back().push_back(a);
			}
			else
			{
				++next_state;
				a.state = next_state;
				a.value = re[i];
				list<Node> p;
				p.push_back(a);
				tnfa.push_back(p);
			}

			break;
		}
		}
		i++;
	}
	modifyTer(isTer, next_state, index);//把最后一个状态置为终结态
	//处理栈内符号
	a = status.top();
	while (a.state != -1)
	{
		if (a.state == '|')
		{
			int prei = a.value - 1;
			for (list<Node>::iterator pi = tnfa[prei].begin(); pi != tnfa[prei].end(); pi++)
			{
				if (pi->state == prei + 1)
					pi->state = next_state;
			}
		}
		status.pop();
		a = status.top();
	}
	tnfa.resize(next_state + 1);
	return;
}

void modifyTer(vector<int>& is_t, unsigned int state, int value)
{
	if (state >= is_t.size())
	{
		is_t.resize(state + 1);
	}
	is_t[state] = value;
}

void joinNfa(vector<list<Node> >& nfa1, const vector<list<Node> >& nfa2)
{
	//这个问题的算法比较简单，只需简单地将第二个nfa的开始的点中的内容全部拷贝
	//给第一个nfa的开始结点然后，再把第二个nfa中除了开始点以外的点连接到第一个
	//nfa的末尾即可。注意，此处要将第二个nfa的结点编号变一下。

	//首先将开始结点合并。
	if (nfa1.empty())
	{
		nfa1 = nfa2;
		return;
	}
	Node en;
	en.state = nfa1.size();//在nfa1的开始结点中加入一个状态点，边上值为epslong，指向nfa2
	en.value = EPSLONG;//然后把nfa2的所有点复制到nfa1。
	nfa1[0].push_back(en);
	//将剩下的内容合并
	size_t cons = nfa1.size();
	copy(nfa2.begin(), nfa2.end(), back_inserter(nfa1));
	//调整合并好的内容中的结点值
	for (unsigned int i = cons; i < (int)nfa1.size(); i++)
	{
		for (list<Node>::iterator p = nfa1[i].begin(); p != nfa1[i].end(); p++)
		{
			p->state += cons;
		}
	}
}

void joinIster(vector<int>& is_t1, const vector<int>& is_t2)
{
	copy(is_t2.begin(), is_t2.end(), back_inserter(is_t1));
}

void Nfa2Dfa()
{
	if (nfa.size() == 1)
	{
		dfa = nfa;
		dfaIsTer = nfaIsTer;
		dfaTer2Action = nfaTer2Action;
		return;
	}
	pair<set<int>, int> ap;
	set<int> I0;
	queue<set<int> > Q;//用来存放已经生成的状态集，并用于判断是否结束。
	map<int, set<int> > valueTable;//用来存放以边上权值为关键字的map，其中对应内容为相应的新的状态集。

	I0.insert(0);//将初始状态放入首字符集内。
	Eclosure(I0);//求Eclosure闭包	

	int current_state = 0;
	ap.first = I0;
	ap.second = current_state;
	dfanodetable.insert(ap);
	Q.push(I0);
	do
	{
		set<int> It = Q.front();
		Q.pop();
		valueTable.clear();//清空以进行新一轮的判断
		//取出新的一个状态集进行判断，如果当中包含了nfa的终态，则需要进行特殊处理。
		for (set<int>::iterator p = It.begin(); p != It.end(); p++)
		{
			int s = *p;
			unsigned int state = dfanodetable[It];
			if (dfa.size() <= state)//如果dfa的空间不足，则扩大它的空间。并且直接
								 //把新的状态存入到dfa中。否则就直接放入就可以。
			{
				dfa.resize(state + 1);
				dfaIsTer.resize(state + 1);
			}
			dfaIsTer[state] = dfaIsTerminated(It);//确定该状态集是不是一个终态
			for (list<Node>::iterator i = nfa[s].begin(); i != nfa[s].end(); i++)
			{
				//每做一次，完成一个新的状态集的生成，
				//并同时应完成相应DFA的构造
				//以下的判断是对每一状态点只对每种边做一次判断。
				if (!valueTable.count(i->value) && i->value != EPSLONG)
				{
					set<int> item;
					item = move(It, i->value);
					Eclosure(item);
					if (!dfanodetable.count(item))
					{
						Q.push(item);
						current_state++;
						pair<set<int>, int> ap;
						ap.first = item;
						ap.second = current_state;
						dfanodetable.insert(ap);
					}
					//此处可能会有所重复，应为当dfanodetable中已经含有相应的状态集的时候，
					//这里可能还是会有操作，多余。
					pair<int, set<int> > tp;
					tp.first = i->value;
					tp.second = item;
					valueTable.insert(tp);
					//下面构造相应的DFA
					Node n0;
					n0.state = dfanodetable[item];
					n0.value = i->value;

					dfa[state].push_back(n0);
				}
				else
				{
					//应该无内容
				}
			}
		}
	} while (!Q.empty());
	//dfa.resize(dfa.size()+1);
	//处理完毕，DFA生成，同时也产生对应action的dfa终态表
	//此处有一个问题，即如何解决临界问题
}


void Eclosure(set<int>& T)//对set<int> &T本身进行操作，将其扩为Eclosure(T)
{
	stack<int> M;
	for (set<int>::iterator p = T.begin(); p != T.end(); p++)
	{
		M.push(*p);
	}
	while (!M.empty())
	{
		int s = M.top();
		M.pop();
		for (list<Node>::iterator p = nfa[s].begin(); p != nfa[s].end(); p++)
		{
			if (p->value == EPSLONG && (!T.count(p->state)))
			{
				M.push(p->state);
				T.insert(p->state);
			}
		}
	}
}

set<int> move(const set<int>& I, int value)//从集合里面读，然后进行move()计算
{
	set<int> r;
	for (set<int>::const_iterator p = I.begin(); p != I.end(); p++)
	{
		int s = *p;
		for (list<Node>::iterator i = nfa[s].begin(); i != nfa[s].end(); i++)
		{
			if (i->value == value && (!r.count(i->state)))
			{
				r.insert(i->state);
			}
		}
	}
	return r;
}

int dfaIsTerminated(set<int>& I)//判断一个状态集是不是终态状态集
{
	for (set<int>::iterator p = I.begin(); p != I.end(); p++)
	{
		if (nfaIsTer[*p])
		{
			return nfaTer2Action[*p];
		}
	}
	return 0;
}

void minidfa()
{
	if (dfa.size() == 1) return;
	vector<set<int> > A(dfa.size());//存储结点对应所有边的对应状态点，索引值为结点编号。个数与结点数一样多。
	/**
	*下面完成的是划分的方法，主要方法是将所有的DFA中的结点对每条边进行扫描，
	*当扫描到某点某条边时，如i点的a边。求其dfa_move()的值，即边所对应下一状态的值。然后把
	*这个值存储到此点所对应的集合中去，即A[i].insert(a);
	*问题：
	*有待改进的地方，这样做只能做到最大划分，但是无法对已经划分好的等价类再进行合并，有关
	*合并方法再做研究。
	**/
	bool is_modified = false;
	do
	{
		is_modified = false;
		for (unsigned int i = 0; i < dfa.size(); i++)//外层循环，表示对每个结点做一次对所有边的搜索
		{
			for (list<Node>::iterator p = dfa[i].begin(); p != dfa[i].end(); p++)
			{	//内层循环，表示对特定结点特定边做一次扫描，
				//找出其对应的下个状态点
				if (!A[i].count(p->state))
					A[i].insert(p->state);
			}
		}
		//以上完成对i号结点的对应下一条边的状态集A[i]的生成。
	//下面开始依据划分对DFA进行修改。
	//第一步，存储所有的点，按照是不是有相同的生成集合
		map<set<int>, vector<int> > dfa2mMap;
		vector<int> node_reserved(dfa.size());
		for (unsigned int j = 0; j < node_reserved.size(); j++)
		{
			node_reserved[j] = 0;//初值赋为假
		}
		for (int i = 0; i < dfa.size(); i++)
		{
			if (dfa2mMap.count(A[i]) && cmpList(dfa[i], dfa[dfa2mMap[A[i]][0]]))//
			{
				if (dfaIsTer[i] == dfaIsTer[dfa2mMap[A[i]][0]])//当两个结点的对应终态表头相同时才能合并
				{
					is_modified = true;
					dfa2mMap[A[i]].push_back(i);
				}
				else if (dfaIsTer[i] != 0)//首先判断是为终态点。为防止两个不同终态动作的符号合并，作此修改
				{
					A[i].insert((-1) * i);//修改A[i]的值，确保它与前面不同，使这个点在后面不会被删掉
					dfa2mMap[A[i]].push_back(i);
				}
			}
			else
			{
				dfa2mMap[A[i]].push_back(i);
			}
		}
		//第二步，应该开始扫描dfa2mMap，并且相应的修改DFA
		//应该是扫描原DFA，因为要修改的地方很多。
		//同时也要修改dfa的状态与action对应的表
		if (!is_modified) continue;

		for (map<set<int>, vector<int> >::iterator pi = dfa2mMap.begin(); pi != dfa2mMap.end(); pi++)
		{
			node_reserved[pi->second[0]] = 1;//把其中要保留的点保存起来
		}
		int cons = 0;
		for (unsigned j = 0; j < node_reserved.size(); j++)
		{
			if (node_reserved[j])
			{
				int x = dfa2mMap[A[j]][0];
				dfa2mMap[A[j]][0] -= cons;
			}
			else
			{
				dfaIsTer.erase(dfaIsTer.begin()+(j - cons));//为什么　会有错？
				dfa.erase(dfa.begin()+(j - cons));
				cons++;//如果当前点不需保留，则将cons++
			}
		}
		for (int i = 0; i < dfa.size(); i++)
		{
			for (list<Node>::iterator p = dfa[i].begin(); p != dfa[i].end(); p++)
			{
				p->state = dfa2mMap[A[p->state]][0];
			}
		}
	} while (is_modified);
	//DFA修改完毕，此时只是完成了最大划分，但还没有做一些合并
}


void genAnalysisCode()//生成条件控制表示的DFA的代码
{
	ofile << "using namespace std;" << endl;
	ofile << "const int START=0;" << endl;
	ofile << "const int ERROR=32767;" << endl;//暂时用-32767定义
	ofile << endl;
	ofile << "int analysis(char *yytext,int n)\n";
	ofile << "{\n";
	ofile << "\tint state=START;\n";
	ofile << "\tint N=n+1;//N表示串长加1,为与状态数保持一致。\n";
	ofile << "\tfor(int i=0;i<N;i++)\n";
	ofile << "\t{\n";
	ofile << "\tswitch(state)\n";
	ofile << "\t{\n";
	//下面开始进入实际构造阶段
	//
	for (unsigned int i = 0; i < dfa.size(); i++)
	{
		ofile << "\tcase " << i << ":\n";
		ofile << "\t{\n";
		if (dfaIsTer[i] > 0)
		{
			ofile << "\t\tif(i==N-1)\n";
			ofile << "\t\t{\n";
			//此处应当打印出识别出该终态时相应的动作。
			size_t length = actionTable[dfaIsTer[i]].size();
			ofile << "\t\t\t" << actionTable[dfaIsTer[i]].substr(1, length - 2)
				<< endl;
			ofile << "\t\t\tbreak;\n";
			ofile << "\t\t}\n";
		}
		for (list<Node>::iterator p = dfa[i].begin(); p != dfa[i].end(); p++)
		{
			ofile << "\t\tif(yytext[i]=='" << (char)p->value << "')\n";
			ofile << "\t\t{\n";
			ofile << "\t\t\tstate=" << p->state << ";\n";
			ofile << "\t\t\tbreak;\n";
			ofile << "\t\t}\n";
		}
		if (dfa[i].size() > 0)
		{
			ofile << "\telse\n";
			ofile << "\t{\n";
			ofile << "\t\treturn ERROR;\n";
			ofile << "\t}\n";
		}
		ofile << "\tbreak;\n";
		ofile << "\t}\n";
	}
	ofile << "\t}\n";
	ofile << "\t}\n";
	ofile << "\n}";
}

bool cmpList(const list<Node>& l1, const list<Node>& l2)
{
	if (l1.size() != l2.size()) return false;
	for (list<Node>::const_iterator p1 = l1.begin(), p2 = l2.begin(); p1 != l1.end() && p2 != l2.end(); p1++, p2++)
		if (p1->state != p2->state || p1->value != p2->value)
			return false;
	return true;
}