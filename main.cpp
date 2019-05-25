#include <iostream>
#include <string>
#include <queue>
#include <stack>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <deque>
#include <iterator>

const std::unordered_map<std::string, int> opset = { //token, precedence pairs
	{"+", 0},
	{"-", 0},
	{"*", 1},
	{"/", 1}
};

const std::unordered_map<std::string, bool> opleftassoc = { //token, left-association (true if operator is left associative) pairs
	{"+", 0},
	{"-", 1},
	{"*", 0},
	{"/", 1}
};

bool is_num(const std::string& s)
{
	for (char c : s) { if (!isdigit(c)) return false; }
}

enum class TokenType { num, op, name, lparen, rparen };

class ParseToken
{
public:
	std::string val;
	TokenType tt;
	int precedence; //meaningless if tt is not TokenType::op
	bool leftassoc = false;
	ParseToken(std::string _val, TokenType _tt, int _precedence = 0, bool _leftassoc = 0) : val(_val), tt(_tt), precedence(_precedence), leftassoc(_leftassoc) {};

	bool operator<(const ParseToken& pt) { return precedence < pt.precedence; }
	bool operator>(const ParseToken& pt) { return precedence > pt.precedence; }
	bool operator>=(const ParseToken& pt) { return precedence >= pt.precedence; }
	bool operator<=(const ParseToken& pt) { return precedence <= pt.precedence; }
};


class TokenQueue { //TokenQueue is an object used by the Shunting-Yard in this implementation that results in a more readable and faster Shunting-Yard
	std::queue<ParseToken> data;
public:
	TokenQueue(std::string s) {
		std::string temp = "";
		for (char c : s) {
			if (opset.count(std::string(1, c)) || c == ')' || c == '(') { //General check if c is an operator if so we want to clear our temp string, push it, then push the operator
				if (temp.length() > 0) {
					if (is_num(temp)) //Check if temp is numeric
						data.push(ParseToken(temp, TokenType::num));
					else
						data.push(ParseToken(temp, TokenType::name));

					temp.clear();
				}
				switch (c) {
				case '(':
					data.push(ParseToken(std::string(1, c), TokenType::lparen));
					break;
				case ')':
					data.push(ParseToken(std::string(1, c), TokenType::rparen));
					break;
				default:
					std::string opss = std::string(1, c);
					data.push(ParseToken(std::string(1, c), TokenType::op, opset.at(opss), opleftassoc.at(opss)));
				}

			}
			else {
				temp.push_back(c);
			}
		}
		if (temp.length() > 0) {
			if (is_num(temp))
				data.push(ParseToken(temp, TokenType::num));
			else
				data.push(ParseToken(temp, TokenType::name));

			temp.clear();
		}
	}

	ParseToken pop()
	{
		ParseToken temp = data.front();
		data.pop();
		return temp;
	}

	bool empty() { return data.empty(); }
};

std::queue<ParseToken> shunting_yard(TokenQueue tq) //Actual Shunting-Yard algorithm. output is the output queue
{
	std::queue<ParseToken> out_q; //output queue
	std::stack<ParseToken> op_stk; //operator stack

	while (!tq.empty()) {
		ParseToken pt = tq.pop();
		switch (pt.tt) {

		case TokenType::num:
			out_q.push(pt);
			break;

		case TokenType::name:
			out_q.push(pt);
			break;

		case TokenType::op:
			while (!op_stk.empty() && ((op_stk.top().tt == TokenType::op || op_stk.top().tt == TokenType::rparen) &&
				(op_stk.top() > pt || (op_stk.top().precedence == pt.precedence && pt.leftassoc)))) {
				out_q.push(op_stk.top());
				op_stk.pop();
			}
			op_stk.push(pt);
			break;

		case TokenType::lparen:
			op_stk.push(pt);
			break;

		case TokenType::rparen:
			while (op_stk.top().tt != TokenType::lparen) {
				out_q.push(op_stk.top());
				op_stk.pop();
			}
			if (op_stk.top().tt == TokenType::lparen) {
				op_stk.pop();
			}
		}
	}
	if (!op_stk.empty()) {
		while (!tq.empty()) {
			out_q.push(tq.pop());
		}
	}
	if (tq.empty()) {
		while (!op_stk.empty()) {
			out_q.push(op_stk.top());
			op_stk.pop();
		}
	}
	return out_q;
}

struct TokenNode { //Node for our ParseTree
	ParseToken data;
	std::vector<TokenNode*> branches;
	TokenNode(ParseToken _data, std::vector<TokenNode*> _branches = {}) : data(_data), branches(_branches) {}
	~TokenNode() { for (TokenNode* tn : branches) { delete tn; } }
};

struct ParseTree {
	TokenNode* root;

	ParseTree(std::queue<ParseToken> tq) { //Turn a Shunting-Yard output queue into a tree of tokens. This is needed to actually evaluate the expression
		std::stack<TokenNode*> pds; //any nodes not yet childed to an operator are pushed here
		while (!tq.empty()) {
			ParseToken pt = ParseToken(tq.front());
			tq.pop();
			if (pt.tt == TokenType::op) { //When we find an operator we must pop n tokens off of pds. n=amount of operands required by given operator or function
				std::vector<TokenNode*> temp;
				for (int i = 0; i < 2; ++i) { //Temporary - solution must be implemented once functions are here. Loop length should depend on input requirement from function
					temp.push_back(pds.top());
					pds.pop();
				}
				std::reverse(std::begin(temp), std::end(temp)); //vector must be reversed in order for the variables to be in the 'right' order
				pds.push(new TokenNode(pt, temp)); //create and push operator node with operand children
			}
			else {
				pds.push(new TokenNode(pt)); //if not an operator, push to pds
			}
		}
		root = pds.top(); //remaining node is the root
	}

	~ParseTree() {
		delete root;
	}
};
//definitions for operator functions
double add(std::vector<double> vec) { return vec[0] + vec[1]; }
double sub(std::vector<double> vec) { return vec[0] - vec[1]; }
double mult(std::vector<double> vec) { return vec[0] * vec[1]; }
double div(std::vector<double> vec) { return vec[0] / vec[1]; }

typedef double (*opfunc)(std::vector<double>);

std::unordered_map<std::string, opfunc > mathmap = {
	{"+", add},
	{"-", sub},
	{"*", mult},
	{"/", div}
};

//Recursive function for evaluating our parse tree
double eval_tree(const TokenNode* tn) {
	if (tn->data.tt == TokenType::op) {
		if (mathmap.count(tn->data.val)) {
			std::vector<double> res;
			for (TokenNode* t : tn->branches) {
				res.push_back(eval_tree(t));
			}
			return mathmap[tn->data.val](res);
		}
	}
	else {
		return atol(tn->data.val.c_str());
	}
}


int main()
{
	std::string s = "(((3*2)+(20/4)/5)+(17/2))/12";
	TokenQueue tq(s);
	std::queue<ParseToken> oq = shunting_yard(tq);
	ParseTree tt(oq);
	int i = 0;
	while (!oq.empty()) {
		std::cout << oq.front().val << " ";
		oq.pop();
	}
	std::cout << std::endl;
	std::cout << eval_tree(tt.root) << std::endl;
	return 0;
}