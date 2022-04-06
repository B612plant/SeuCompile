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
#define LAYER_ID 15//��ʶ%%
#define HEADER_BEGIN 16//��ʶ%{%}
#define HEADER_END 17//
#define BEGIN 0
#define ERROR -11
#define EPSLONG -1
using namespace std;
//������ṹ
struct Node
{
	unsigned int value;
	unsigned int state;
};
//����һ�鳣��
ifstream ifile;
ofstream ofile;
int lineno = 0;
vector<list<Node> > nfa;
vector<list<Node> > dfa;
unordered_map<string, string> id2reTable;//�洢������б�ʶ��������ʽ��ӳ��
unordered_map<int, int> nfaTer2Action;//�洢NFA��̬��action��ͷ��Ӧ���ݡ�
unordered_map<int, int> dfaTer2Action;//�洢DFA��̬��action��ͷ��Ӧ���ݣ�����������Nfa2Dfa()ʱ���
vector<string> actionTable;//�洢action�ڶ�Ӧ����
vector<int> nfaIsTer;
map< set<int>, int > dfanodetable;
vector<int> dfaIsTer;//���dfa����Щ���ս�̬��


//����һЩ����
int checkSpecsign(char c);
void compleRe(string& re);
void produceNfa(const string& re, vector<list<Node> >& tnfa, vector<int>& isTer, int index);
void modifyTer(vector<int>& is_t, unsigned int state, int value);
void joinNfa(vector<list<Node> >& nfa1, const vector<list<Node> >& nfa2);
void joinIster(vector<int>& is_t1, const vector<int>& is_t2);
void Nfa2Dfa();
void Eclosure(set<int>& T);
set<int> move(const set<int>& I, int value);
int dfaIsTerminated(set<int>& I);//������̬��Ӧ����������Ƿ���̬����-1
void minidfa();
void genAnalysisCode();//���ɴʷ��������ֵĴ���
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
	//�жϿ�ͷ�ǲ���%{
	if (state != HEADER_BEGIN)
	{
		cout << "The input file has no correct formation,Please try again!\n";
		return;
	}
	//�жϵ�%}���ļ�βΪֹ����ɨ��
	while (!ifile.eof() && state != HEADER_END)
	{
		c = ifile.get();
		if (c == '\t') continue;//����\t�ַ��������
		if (c == '%') { state = checkSpecsign(c); continue; }//�����ܵ�%ʱ��ע���ж��ǲ����������
		if (c == '\n') lineno++;//���к������������жϴ����кš�
		ofile.put(c);
	}
	//������ɶ������%{ %}֮���ɨ��
	//���¿�ʼ�Զ���õĹؼ��ֵ�������ʽ��ɨ�裬������洢������
	ifile.get();
	pair<string, string> pi;
	state = BEGIN;

	while (!ifile.eof() && state != LAYER_ID)//�˴��Ĵ��������⣬��Ҫ����һ��������
	{
		c = ifile.get();
		if (c == '%')
		{
			state = checkSpecsign(c);
			if (state == ERROR)//�û��Զ���ı�ʶ�������Ժ��д������ַ�%��
			{
				cerr << "There is an error in line " << lineno << " !" << endl;
				return;
			}
			continue;//����������������Ĵ�����Ϊ�Ѿ����սᡣ
		}
		else
		{
			ifile.unget();//������������ַ�%���ѵ�ǰ�ַ��Ż����С�
		}
		string id, re;
		ifile >> id >> re;
		pi.first = id;
		pi.second = re;
		id2reTable.insert(pi);
		lineno++;
		ifile.get();
	}
	//�����ǶԶ���ε�ɨ��
	//�����ǶԹ���ε�ɨ�����
	state = BEGIN;
	actionTable.push_back("begin");//ʹaction���1�ŵ�Ԫ��ʼ�����ڴ���
	ifile.get();
	while (!ifile.eof() && state != LAYER_ID)
	{
		c = ifile.get();
		if (c == '%')
		{
			state = checkSpecsign(c);
			if (state == ERROR)//�û��Զ���ı�ʶ�������Ժ��д������ַ�%��
			{
				cerr << "There is an error in line " << lineno << " !" << endl;
				return;
			}
			continue;//����������������Ĵ�����Ϊ�Ѿ����սᡣ
		}
		else
		{
			ifile.unget();//������������ַ�%���ѵ�ǰ�ַ��Ż����С�
		}
		string onestr;
		string re, action;
		//����һ������ֻ��д��һ���ڡ�
		getline(ifile, onestr);
		string delim = " \t";
		size_t offset = onestr.find_first_of(delim);
		re = onestr.substr(0, offset);
		while (onestr[offset] == ' ' || onestr[offset] == '\t') offset++;
		action = onestr.substr(offset, onestr.size() - offset);
		actionTable.push_back(action);
		//���濪ʼ������ʽ��ɨ���滻,���ѵ��к����û��Զ����ʶ���ĵط��滻������������ʽ
		compleRe(re);
		//�滻��ϣ����濪ʼ��������ʽ����NFA
		vector<list<Node> > nfa1;
		vector<int> isTer;
		int actionTableIndex = actionTable.size() - 1;
		produceNfa(re, nfa1, isTer, actionTableIndex);//����nfa������Ӧ����̬��
		joinNfa(nfa, nfa1);//�ϲ�nfa
		unsigned int adjustp = nfaIsTer.size();
		joinIster(nfaIsTer, isTer);//�ϲ�״̬��
		//����action������
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
	//����һ�����NFA����ˣ��洢��nfa�С�
	Nfa2Dfa();//������dfa������dfa���vector�С�
	minidfa();//���������������˶�dfaTer2Action����޸ġ�
	cout << "Generating code......" << endl;
	genAnalysisCode();
	//���濪ʼ�����һ�Σ����û��Զ��������̶ε������
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
	//�˴�Ӧ��ע��һ�����⣬����α�����ѭ���ĳ���
	//���û��Զ���ı�ʶ������ѭ��Ƕ�׶���ʱ����ν���ʶ����򱨴�
	stack<int> status;//״̬ջ
	status.push(-1);
	unsigned int i = 0;
	while (i < re.size())
	{
		if (re[i] == '{')
			status.push(i);//��¼�´�ʱ��λ��
		if (re[i] == '}')
		{
			int prei = status.top();
			if (prei < 0)
			{
				i++;
				continue;//��ʾ��ʱ��}������ԣ����ֱ�����
			}
			int length = i - prei - 1;
			string id = re.substr(prei + 1, length);
			string replacestr = id2reTable[id];
			if (replacestr.empty()) continue;//��ʾ���е����ݲ��Ǳ�ʶ�������Բ�����
			re.replace(prei, length + 2, replacestr);//+2��ʾ����{}һ����
			i = prei - 1;
		}
		i++;
	}
	//������ϣ���ʱ������ʽ��Ӧ��û���û��Զ���ı�ʶ����
}

void produceNfa(const string& re, vector<list<Node> >& tnfa, vector<int>& isTer, int index)
{
	stack<Node> status;//״̬ջ�����ڴ洢��ǰ״̬
	stack<int> tstatus;
	bool is_reduce = false;//�ж��ǲ����Ѿ���Լ�ˡ�
	bool isNonSpec = false;
	unsigned int next_state = 0;
	Node a;
	a.state = -1;
	a.value = -1;
	status.push(a);
	unsigned int i = 0;
	list<Node> p;
	//tnfa.push_back(p);//��ʼʱ������tnfa�ڷ���һ��list<Node>
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
			a.state = re[i];//��state��ʾ״̬�ַ�
			a.value = i;//��value��ʾ��ʱ������ֵ
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
			a.value = tnfa.size() - 1;//ʹvalue�ڴ洢��ǰnfa�е����״ֵ̬��
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
			while (a.state == '|')//���ջ���ǣ�,��������̬�ĵ���
			{
				int prestate = a.value;
				prestate--;//��ʱ��ֵ��ʾ��Ҫ�޸ĵ�״̬�㡣
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
				int prei = a.value;//ȡ�õ��ǵ�ʱnfa�е�״ֵ̬��
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
			a.state = next_state;//��nfa�ڴ洢һ���ձߣ�Ϊ�˷�����й��졣
			a.value = EPSLONG;

			tnfa[prestate].push_back(a);
			a.state = '|';
			a.value = next_state;//�洢��ʱ������ǰһ״̬��ֵ
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
			while (a.state == '|')//���ջ���ǣ�,��������̬�ĵ���
			{
				int prestate = a.value;
				prestate--;//��ʱ��ֵ��ʾ��Ҫ�޸ĵ�״̬�㡣
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
				i = a.value;//�ں��������1��
				is_reduce = true;//����Ϊ�Ѿ���Լ����ʼ����
				++next_state;//������һ״ֵ̬
			}
			else if (a.state == -1)
			{//�������ò���
				++next_state;
				list<Node> p;
				a.value = re[i];
				a.state = next_state;
				p.push_back(a);
				tnfa.push_back(p);
				modifyTer(isTer, next_state, 0);
			}//�������ʲô����������
			break;
		}
		case '-':
		{
			a = status.top();
			list<Node> p;
			if (i >= 1 && i < re.size() - 1 && re[i - 1] < re[i + 1])
			{
				if (a.state != '[')//���״̬ջΪ�գ���ʾ������㣬����ֱ������
				{
					for (char j = re[i - 1]; j < re[i + 1]; j++)
					{
						++next_state;
						a.state = next_state;
						a.value = j;
						p.push_back(a);
						tnfa.push_back(p);//ÿ�ζ�����һ��״̬��
					}
				}
				else
				{
					if (is_reduce)
					{
						for (char j = re[i - 1] + 1; j < re[i + 1]; j++)//re[i-1]+1����Ϊre[i]�Ѿ�������
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
	modifyTer(isTer, next_state, index);//�����һ��״̬��Ϊ�ս�̬
	//����ջ�ڷ���
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
	//���������㷨�Ƚϼ򵥣�ֻ��򵥵ؽ��ڶ���nfa�Ŀ�ʼ�ĵ��е�����ȫ������
	//����һ��nfa�Ŀ�ʼ���Ȼ���ٰѵڶ���nfa�г��˿�ʼ������ĵ����ӵ���һ��
	//nfa��ĩβ���ɡ�ע�⣬�˴�Ҫ���ڶ���nfa�Ľ���ű�һ�¡�

	//���Ƚ���ʼ���ϲ���
	if (nfa1.empty())
	{
		nfa1 = nfa2;
		return;
	}
	Node en;
	en.state = nfa1.size();//��nfa1�Ŀ�ʼ����м���һ��״̬�㣬����ֵΪepslong��ָ��nfa2
	en.value = EPSLONG;//Ȼ���nfa2�����е㸴�Ƶ�nfa1��
	nfa1[0].push_back(en);
	//��ʣ�µ����ݺϲ�
	size_t cons = nfa1.size();
	copy(nfa2.begin(), nfa2.end(), back_inserter(nfa1));
	//�����ϲ��õ������еĽ��ֵ
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
	queue<set<int> > Q;//��������Ѿ����ɵ�״̬�����������ж��Ƿ������
	map<int, set<int> > valueTable;//��������Ա���ȨֵΪ�ؼ��ֵ�map�����ж�Ӧ����Ϊ��Ӧ���µ�״̬����

	I0.insert(0);//����ʼ״̬�������ַ����ڡ�
	Eclosure(I0);//��Eclosure�հ�	

	int current_state = 0;
	ap.first = I0;
	ap.second = current_state;
	dfanodetable.insert(ap);
	Q.push(I0);
	do
	{
		set<int> It = Q.front();
		Q.pop();
		valueTable.clear();//����Խ�����һ�ֵ��ж�
		//ȡ���µ�һ��״̬�������жϣ�������а�����nfa����̬������Ҫ�������⴦��
		for (set<int>::iterator p = It.begin(); p != It.end(); p++)
		{
			int s = *p;
			unsigned int state = dfanodetable[It];
			if (dfa.size() <= state)//���dfa�Ŀռ䲻�㣬���������Ŀռ䡣����ֱ��
								 //���µ�״̬���뵽dfa�С������ֱ�ӷ���Ϳ��ԡ�
			{
				dfa.resize(state + 1);
				dfaIsTer.resize(state + 1);
			}
			dfaIsTer[state] = dfaIsTerminated(It);//ȷ����״̬���ǲ���һ����̬
			for (list<Node>::iterator i = nfa[s].begin(); i != nfa[s].end(); i++)
			{
				//ÿ��һ�Σ����һ���µ�״̬�������ɣ�
				//��ͬʱӦ�����ӦDFA�Ĺ���
				//���µ��ж��Ƕ�ÿһ״̬��ֻ��ÿ�ֱ���һ���жϡ�
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
					//�˴����ܻ������ظ���ӦΪ��dfanodetable���Ѿ�������Ӧ��״̬����ʱ��
					//������ܻ��ǻ��в��������ࡣ
					pair<int, set<int> > tp;
					tp.first = i->value;
					tp.second = item;
					valueTable.insert(tp);
					//���湹����Ӧ��DFA
					Node n0;
					n0.state = dfanodetable[item];
					n0.value = i->value;

					dfa[state].push_back(n0);
				}
				else
				{
					//Ӧ��������
				}
			}
		}
	} while (!Q.empty());
	//dfa.resize(dfa.size()+1);
	//������ϣ�DFA���ɣ�ͬʱҲ������Ӧaction��dfa��̬��
	//�˴���һ�����⣬����ν���ٽ�����
}


void Eclosure(set<int>& T)//��set<int> &T������в�����������ΪEclosure(T)
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

set<int> move(const set<int>& I, int value)//�Ӽ����������Ȼ�����move()����
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

int dfaIsTerminated(set<int>& I)//�ж�һ��״̬���ǲ�����̬״̬��
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
	vector<set<int> > A(dfa.size());//�洢����Ӧ���бߵĶ�Ӧ״̬�㣬����ֵΪ����š�����������һ���ࡣ
	/**
	*������ɵ��ǻ��ֵķ�������Ҫ�����ǽ����е�DFA�еĽ���ÿ���߽���ɨ�裬
	*��ɨ�赽ĳ��ĳ����ʱ����i���a�ߡ�����dfa_move()��ֵ����������Ӧ��һ״̬��ֵ��Ȼ���
	*���ֵ�洢���˵�����Ӧ�ļ�����ȥ����A[i].insert(a);
	*���⣺
	*�д��Ľ��ĵط���������ֻ��������󻮷֣������޷����Ѿ����ֺõĵȼ����ٽ��кϲ����й�
	*�ϲ����������о���
	**/
	bool is_modified = false;
	do
	{
		is_modified = false;
		for (unsigned int i = 0; i < dfa.size(); i++)//���ѭ������ʾ��ÿ�������һ�ζ����бߵ�����
		{
			for (list<Node>::iterator p = dfa[i].begin(); p != dfa[i].end(); p++)
			{	//�ڲ�ѭ������ʾ���ض�����ض�����һ��ɨ�裬
				//�ҳ����Ӧ���¸�״̬��
				if (!A[i].count(p->state))
					A[i].insert(p->state);
			}
		}
		//������ɶ�i�Ž��Ķ�Ӧ��һ���ߵ�״̬��A[i]�����ɡ�
	//���濪ʼ���ݻ��ֶ�DFA�����޸ġ�
	//��һ�����洢���еĵ㣬�����ǲ�������ͬ�����ɼ���
		map<set<int>, vector<int> > dfa2mMap;
		vector<int> node_reserved(dfa.size());
		for (unsigned int j = 0; j < node_reserved.size(); j++)
		{
			node_reserved[j] = 0;//��ֵ��Ϊ��
		}
		for (int i = 0; i < dfa.size(); i++)
		{
			if (dfa2mMap.count(A[i]) && cmpList(dfa[i], dfa[dfa2mMap[A[i]][0]]))//
			{
				if (dfaIsTer[i] == dfaIsTer[dfa2mMap[A[i]][0]])//���������Ķ�Ӧ��̬��ͷ��ͬʱ���ܺϲ�
				{
					is_modified = true;
					dfa2mMap[A[i]].push_back(i);
				}
				else if (dfaIsTer[i] != 0)//�����ж���Ϊ��̬�㡣Ϊ��ֹ������ͬ��̬�����ķ��źϲ��������޸�
				{
					A[i].insert((-1) * i);//�޸�A[i]��ֵ��ȷ������ǰ�治ͬ��ʹ������ں��治�ᱻɾ��
					dfa2mMap[A[i]].push_back(i);
				}
			}
			else
			{
				dfa2mMap[A[i]].push_back(i);
			}
		}
		//�ڶ�����Ӧ�ÿ�ʼɨ��dfa2mMap��������Ӧ���޸�DFA
		//Ӧ����ɨ��ԭDFA����ΪҪ�޸ĵĵط��ܶࡣ
		//ͬʱҲҪ�޸�dfa��״̬��action��Ӧ�ı�
		if (!is_modified) continue;

		for (map<set<int>, vector<int> >::iterator pi = dfa2mMap.begin(); pi != dfa2mMap.end(); pi++)
		{
			node_reserved[pi->second[0]] = 1;//������Ҫ�����ĵ㱣������
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
				dfaIsTer.erase(dfaIsTer.begin()+(j - cons));//Ϊʲô�����д�
				dfa.erase(dfa.begin()+(j - cons));
				cons++;//�����ǰ�㲻�豣������cons++
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
	//DFA�޸���ϣ���ʱֻ���������󻮷֣�����û����һЩ�ϲ�
}


void genAnalysisCode()//�����������Ʊ�ʾ��DFA�Ĵ���
{
	ofile << "using namespace std;" << endl;
	ofile << "const int START=0;" << endl;
	ofile << "const int ERROR=32767;" << endl;//��ʱ��-32767����
	ofile << endl;
	ofile << "int analysis(char *yytext,int n)\n";
	ofile << "{\n";
	ofile << "\tint state=START;\n";
	ofile << "\tint N=n+1;//N��ʾ������1,Ϊ��״̬������һ�¡�\n";
	ofile << "\tfor(int i=0;i<N;i++)\n";
	ofile << "\t{\n";
	ofile << "\tswitch(state)\n";
	ofile << "\t{\n";
	//���濪ʼ����ʵ�ʹ���׶�
	//
	for (unsigned int i = 0; i < dfa.size(); i++)
	{
		ofile << "\tcase " << i << ":\n";
		ofile << "\t{\n";
		if (dfaIsTer[i] > 0)
		{
			ofile << "\t\tif(i==N-1)\n";
			ofile << "\t\t{\n";
			//�˴�Ӧ����ӡ��ʶ�������̬ʱ��Ӧ�Ķ�����
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