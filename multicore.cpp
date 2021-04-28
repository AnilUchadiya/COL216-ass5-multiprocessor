//#include <bits/stdc++.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <boost/algorithm/string.hpp>
#include <filesystem>

using namespace std;


int NUM_CORES;
int MAX_CYCLES;

int grant = 0;

struct multicore_registers{
	int coreno;
    int line_nu = 0;
	int cpi = 0;
	int PC = 0;
	bool exit = false;
    vector<int32_t> s_registers;
    vector<int32_t> t_registers;
    vector<int> line_countt;
    map<string, int> countt;
	vector<lines> assembly_program_storage;
};


// int line_nu = 0;
// // 32 32-bits register
// vector<int32_t> registers;
// vector<int32_t> s_registers;
// vector<int32_t> t_registers;
// vector<int> line_countt;
// map<string, int> countt;





map<string, int> label_map;
vector<vector<int32_t> > dram;

vector<int32_t> buff_row;
int current_row_address = -1;
int row_address, col_address;
int ROW_ACCESS_DELAY;
int COL_ACCESS_DELAY;

int num_of_row_buff_updates = 0;

vector<int> row_addresses;
vector<int> col_addresses;

struct lines
{
	string instruction;
	vector<string> arguements;
	string whole_line;
};

struct dram_requests
{
	string inst;
	string register_name;
	int address;
	int row_address;
	int col_address;
	int dram_request_cpi;
	string whole_line;
};

vector<dram_requests> dram_queue;

bool execute_the_queue_rn = false;

vector<lines> assembly_program_storage;

bool split_inst(string line, unordered_map<string, int> ins_check,struct multicore_registers reg)
{
	boost::trim_left(line);
	boost::trim_right(line);

	vector<string> arg;
	string ar1 = "";
	bool flag = true;
	struct lines l;
	l.whole_line = line;
	if (line[line.length() - 1] == ':')
	{
		label_map[line] = reg.line_nu;
		l.instruction = line;
		assembly_program_storage.push_back(l);
		return true;
	}
	vector<string> argss;
	if (line == "\n" || line == "\0")
	{
		l.instruction = "";
		assembly_program_storage.push_back(l);
		return true;
	}
	for (int i = 0; i < line.length(); i++)
	{
		if (line[i] == ',')
		{
			arg.push_back(ar1);
			ar1 = "";
		}
		else if ((line[i] == ' ') && flag)
		{
			// ar1.push_back(line[i]);
			flag = false;
			l.instruction = ar1;
			ar1 = "";
		}
		else if (line[i] != ' ')
			ar1.push_back(line[i]);
	}
	arg.push_back(ar1);
	l.arguements = arg;
	if (ins_check.find(l.instruction) == ins_check.end())
	{
		std::cout << "Invalid Instruction at line : " << reg.line_nu<< "AND at core : "<<reg.coreno << '\n';
		cout << "Invalid-instruction : " << l.instruction << endl;
		return false;
	}
	if (ins_check[l.instruction] != l.arguements.size())
	{
		cout << "Error: Invalid no of Arguement :-" << l.instruction << "-required " << ins_check[l.instruction] << " arguement provided " << l.arguements.size() << " arguement " << endl;
		return false;
	}

	if(l.arguements[0] == "$zero"){
		cout<<"Error : $zero register is immutable -> "<<line<<endl;
		return false;
	}
	assembly_program_storage.push_back(l);
	return true;
	// return arg;
}

bool empty(std::ifstream &pFile)
{
	return pFile.peek() == std::ifstream::traits_type::eof();
}

/*****************************************************************************including ass4 functions from header file ******************************/


int get_address_value(string s,struct multicore_registers reg)
{
	string offset = "";
	for (int i = 0; i < s.length(); i++)
	{
		if (s[i] != '(')
		{
			offset += s[i];
		}
		else
		{
			break;
		}
	}
	string addr = "";
	for (int i = offset.length() + 3; i < s.length(); i++)
	{
		if (s[i] != ')')
		{
			addr += s[i];
		}
		else
		{
			break;
		}
	}
	int ans;
	if (s[offset.length() + 2] == 's')
	{
		ans = reg.s_registers[stol(addr)];
	}
	else
	{
		ans = reg.t_registers[stol(addr)];
	}
	ans += stol(offset);
	return ans;
}

int time_taken_for_the_printing_of_queue()
{
	int t = 0;
	int bla = 0;
	int current_row_address_temp = current_row_address;
	while (bla != dram_queue.size())
	{
		bla++;
		int time = 0;
		if (dram_queue[0].inst == "sw")
		{
			if (current_row_address_temp == -1)
			{
				time += ROW_ACCESS_DELAY;
				time += COL_ACCESS_DELAY;
				current_row_address_temp = dram_queue[0].row_address;
			}
			else if (current_row_address_temp == dram_queue[0].row_address)
			{
				time += COL_ACCESS_DELAY;
			}
			else
			{
				time += 2 * ROW_ACCESS_DELAY;
				time += COL_ACCESS_DELAY;
				current_row_address_temp = dram_queue[0].row_address;
			}
		}
		else
		{
			if (current_row_address_temp == -1)
			{
				time += ROW_ACCESS_DELAY;
				time += COL_ACCESS_DELAY;
				current_row_address_temp = dram_queue[0].row_address;
			}
			else if (current_row_address_temp == dram_queue[0].row_address)
			{
				time += COL_ACCESS_DELAY;
			}
			else
			{
				time += 2 * ROW_ACCESS_DELAY;
				time += COL_ACCESS_DELAY;
				current_row_address_temp = dram_queue[0].row_address;
			}
		}
		t += time;
	}
	return t;
}

string print_the_queue(struct multicore_registers reg)
{
	string answer = "";
	int cpi_number = reg.cpi;
	int bla = 0;
	while (dram_queue.size() != 0)
	{
		bla++;
		if (cpi_number > dram_queue[0].dram_request_cpi)
		{
			cpi_number = cpi_number;
		}
		else
		{
			cpi_number = dram_queue[0].dram_request_cpi;
		}
		if (bla == 1)
		{
			cpi_number = dram_queue[0].dram_request_cpi;
		}
		int time = 0;
		string row_update_info = "";
		if (dram_queue[0].inst == "sw")
		{

			if (dram_queue[0].register_name[1] == 's')
				{
					dram[dram_queue[0].row_address][dram_queue[0].col_address] = reg.s_registers[stol(dram_queue[0].register_name.substr(2))];
				}
				else
				{
					dram[dram_queue[0].row_address][dram_queue[0].col_address] = reg.t_registers[stol(dram_queue[0].register_name.substr(2))];
				}
			if (current_row_address == -1)
			{
				//dram[row_address][col_address]=registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(1))];
				time += ROW_ACCESS_DELAY;
				time += COL_ACCESS_DELAY;
				current_row_address = dram_queue[0].row_address;
				row_update_info += "; row buffer updated, row: ";
				row_update_info += to_string(dram_queue[0].row_address);
				row_update_info += ", col offset: ";
				row_update_info += to_string(dram_queue[0].col_address);
				num_of_row_buff_updates += 2;
			}
			else if (current_row_address == dram_queue[0].row_address)
			{
				//dram[row_address][col_address]=registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(1))];
				time += COL_ACCESS_DELAY;
				row_update_info += "; row buffer containg the same row: ";
				row_update_info += to_string(dram_queue[0].row_address);
				row_update_info += ", col offset: ";
				row_update_info += to_string(dram_queue[0].col_address);
				num_of_row_buff_updates += 1;
			}
			else
			{
				//dram[row_address][col_address]=registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(1))];
				time += 2 * ROW_ACCESS_DELAY;
				time += COL_ACCESS_DELAY;
				row_update_info += "; row buffer got updated, from row: ";
				row_update_info += to_string(current_row_address);
				row_update_info += " to row: ";
				row_update_info += to_string(dram_queue[0].row_address);
				row_update_info += ", col offset: ";
				row_update_info += to_string(dram_queue[0].col_address);
				current_row_address = dram_queue[0].row_address;
				num_of_row_buff_updates += 2;
			}

			cpi_number += time;
			if (dram_queue[0].register_name[1] == 's')
			{
				answer += "clock cycle " + to_string(cpi_number - time + 1) + "-" + to_string(cpi_number) + "; " + dram_queue[0].whole_line + "; memory address " + to_string(dram_queue[0].address) + "-" + to_string(dram_queue[0].address + 3) + "= " + to_string(reg.s_registers[stol(dram_queue[0].register_name.substr(2))]) + row_update_info + "\n";
			}
			else
			{
				answer += "clock cycle " + to_string(cpi_number - time + 1) + "-" + to_string(cpi_number) + "; " + dram_queue[0].whole_line + "; memory address " + to_string(dram_queue[0].address) + "-" + to_string(dram_queue[0].address + 3) + "= " + to_string(reg.t_registers[stol(dram_queue[0].register_name.substr(2))]) + row_update_info + "\n";
			}
		}
		else
		{
			if (dram_queue[0].register_name[1] == 's')
			{
				reg.s_registers[stol(dram_queue[0].register_name.substr(2))] = dram[dram_queue[0].row_address][dram_queue[0].col_address];
			}
			else
			{
				reg.t_registers[stol(dram_queue[0].register_name.substr(2))] = dram[dram_queue[0].row_address][dram_queue[0].col_address];
			}
			if (current_row_address == -1)
			{
				//registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(1))]= dram[row_address][col_address];
				time += ROW_ACCESS_DELAY;
				time += COL_ACCESS_DELAY;
				current_row_address = dram_queue[0].row_address;
				row_update_info += "; row buffer updated, row: ";
				row_update_info += to_string(dram_queue[0].row_address);
				row_update_info += ", col offset: ";
				row_update_info += to_string(dram_queue[0].col_address);
				num_of_row_buff_updates += 1;
			}
			else if (current_row_address == dram_queue[0].row_address)
			{
				//registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(1))]= dram[row_address][col_address];
				time += COL_ACCESS_DELAY;
				row_update_info += "; row buffer containg the same row: ";
				row_update_info += to_string(dram_queue[0].row_address);
				row_update_info += ", col offset: ";
				row_update_info += to_string(dram_queue[0].col_address);
				num_of_row_buff_updates += 0;
			}
			else
			{
				//registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(1))]= dram[row_address][col_address];
				time += 2 * ROW_ACCESS_DELAY;
				time += COL_ACCESS_DELAY;
				row_update_info += "; row buffer got updated, from row: ";
				row_update_info += to_string(current_row_address);
				row_update_info += " to row: ";
				row_update_info += to_string(dram_queue[0].row_address);
				row_update_info += ", col offset: ";
				row_update_info += to_string(dram_queue[0].col_address);
				current_row_address = dram_queue[0].row_address;
				num_of_row_buff_updates += 1;
			}
			cpi_number += time;
			if (dram_queue[0].register_name[1] == 's')
			{
				answer += "clock cycle " + to_string(cpi_number - time + 1) + "-" + to_string(cpi_number) + "; " + dram_queue[0].whole_line + "; $s" + (dram_queue[0].register_name.substr(2)) + "=" + to_string(reg.s_registers[stol(dram_queue[0].register_name.substr(2))]) + row_update_info + "\n";
			}
			else
			{
				answer += "clock cycle " + to_string(cpi_number - time + 1) + "-" + to_string(cpi_number) + "; " + dram_queue[0].whole_line + "; $t" + (dram_queue[0].register_name.substr(2)) + "=" + to_string(reg.t_registers[stol(dram_queue[0].register_name.substr(2))]) + row_update_info + "\n";
			}
		}
		dram_queue.erase(dram_queue.begin());
	}
	reg.cpi = cpi_number;
	return answer;
}


bool compare(const dram_requests a, const dram_requests b)
{
	if (a.row_address == current_row_address && b.row_address != current_row_address)
	{
		return true;
	}
	else if (a.row_address != current_row_address && b.row_address == current_row_address)
	{
		return false;
	}
	else if (a.row_address == b.row_address)
	{
		
		return false;
	}
	else
	{
		if(a.inst != b.inst && a.register_name == b.register_name) return false;
		else {return a.row_address < b.row_address;}
	}
}
 
vector<dram_requests> reschedule(vector<dram_requests> drams, int str, int end)
{
	sort(drams.begin(), drams.end(), compare);
	return drams;
}

int ignored_instructions = 0;
vector<dram_requests> reschedule(vector<dram_requests> drams)
{
	vector<dram_requests> rs = drams;
	vector<dram_requests> ans;
	int current = current_row_address;
	int qs = drams.size();
	if (true)
	{
		for (int i = 0; i < qs; i++)
		{
			for (int j = i + 1; j < qs; j++)
			{
				if (rs[j].inst == rs[i].inst && rs[j].address == rs[i].address && rs[j].register_name == rs[i].register_name && rs[i].inst != "-")
				{
					cout << "ignored duplicate instruction : " << rs[j].whole_line << endl;
					ignored_instructions++;
					rs[j].inst = "-";
                }
				else if (rs[j].inst == rs[i].inst && rs[j].address == rs[i].address && rs[j].inst == "sw" && abs(i-j)==1)
				{
					cout << "ignored  instruction : " << rs[i].whole_line << endl;
					rs[i].inst = "-";
				}
				else if (rs[j].inst == rs[i].inst && rs[j].register_name == rs[i].register_name && rs[j].inst == "lw" && abs(i-j)==1)
				{
					cout << "ignored  instruction : " << rs[i].whole_line << endl;
					rs[i].inst = "-";
				}
				
			}
			if (rs[i].inst != "-")
				ans.push_back(rs[i]);
		}
		ans = reschedule(ans, 0, ans.size());
	}
	return ans;
}

#include <glob.h>

vector<string> globVector(const string& pattern){
    glob_t glob_result;
    glob(pattern.c_str(),GLOB_TILDE,NULL,&glob_result);
    vector<string> files;
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        files.push_back(string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    return files;
}

bool multicore_finish(struct multicore_registers multi_reg[]){
	bool ret = true;
	for(int i =0;i<NUM_CORES;i++){
		if(multi_reg[i].assembly_program_storage[multi_reg[i].PC].instruction != "exit:")
		{
			multi_reg[i].exit = true;
		}
		else{
			ret = false;
		}
	}
	return ret;
}
int main(int argc, char **argv)
{


    string path = argv[3];
    NUM_CORES = stoi(argv[1]);
    MAX_CYCLES = stoi(argv[2]);
    vector<string> files  = globVector(path);
	struct multicore_registers multi_reg[NUM_CORES];
    for(auto i : files){
        cout<<i<<endl;
    }
	ROW_ACCESS_DELAY = stoi(argv[4]);
	COL_ACCESS_DELAY = stoi(argv[5]);
	bool reorder = false;
	if (argc == 5)
	{
		reorder = true;
	}

	unordered_map<string, int> ins_check;

	ins_check["add"] = 3;
	ins_check["lw"] = 2;
	ins_check["sw"] = 2;
	ins_check["addi"] = 3;
	ins_check["j"] = 1;
	ins_check["sub"]=3;
	ins_check["mul"]=3;
	ins_check["beq"]=3;
	ins_check["bne"]=3;
	ins_check["slt"]=3;

	/*************************************************************multi core program reading start***************************************/
	for(int core_no =0;core_no<NUM_CORES;core_no++){
		for (int i = 0; i < 8; i++)
		{
			multi_reg[core_no].s_registers.push_back((int32_t)0);
		}
		for (int i = 0; i < 10; i++)
		{
			multi_reg[core_no].t_registers.push_back((int32_t)0);
		}
		string myText;
		ifstream MyReadFile(files[core_no]);

		if (!MyReadFile)
		{
			cerr << "Error File not found : " << argv[1] << '\n';
			return 0;
		}
		if (empty(MyReadFile))
		{
			cerr << "Error Empty file" << '\n';
			return 0;
		}
		/* code */
		bool error = false;
		while (getline(MyReadFile, myText))
		{

			// if(myText=="\0" || myText == "\n") continue;
			multi_reg[core_no].line_nu++;
			if (!split_inst(myText, ins_check,multi_reg[core_no]))
			{
				error = true;
			}
		}
		MyReadFile.close();
		if (error)
		{
			cout << "Invalid Format Please use proper formatted file" << endl;
			return 0;
		}

		for (int i = 0; i < multi_reg[core_no].assembly_program_storage.size() - 1; i++)
		{
			multi_reg[core_no].line_countt.push_back(0);
		}
		MyReadFile.close();
	}
	// for (int i = 0; i < 8; i++)
	// {
	// 	s_registers.push_back((int32_t)0);
	// }
	// for (int i = 0; i < 10; i++)
	// {
	// 	t_registers.push_back((int32_t)0);
	// }

	for (int i = 0; i < 1024; i++)
	{
		vector<int32_t> v;
		for (int j = 0; j < 1024; j++)
		{
			v.push_back(0);
		}
		dram.push_back(v);
	}

	
	//ins_check["exit:"]=0;

	// string myText;
	// ifstream MyReadFile(argv[1]);

	// if (!MyReadFile)
	// {
	// 	cerr << "Error File not found : " << argv[1] << '\n';
	// 	return 0;
	// }
	// if (empty(MyReadFile))
	// {
	// 	cerr << "Error Empty file" << '\n';
	// 	return 0;
	// }
	int lno[NUM_CORES] = {0};
	int core_no =0;
	try
	{
		/* code */
		
		while (multicore_finish(multi_reg))
		{
			/***********************************************multi core program execution**********************************************************************************/
			for(core_no=0;core_no < NUM_CORES;core_no++)
			{
				multi_reg[core_no].countt[assembly_program_storage[multi_reg[core_no].PC].instruction]++;
				lno[core_no]++;
				multi_reg[core_no].line_countt[multi_reg[core_no].PC]++;
				// cout<<assembly_program_storage[multi_reg[core_no].PC].instruction<<" "<<assembly_program_storage[multi_reg[core_no].PC].arguements[0]<<'\n';
				if (assembly_program_storage[multi_reg[core_no].PC].instruction == "")
				{
					multi_reg[core_no].PC++;
					continue;
				}
				if (assembly_program_storage[multi_reg[core_no].PC].instruction == "lw")
				{

					// cout<<stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1])<<endl;
					int time = 0;
					int address = get_address_value(assembly_program_storage[multi_reg[core_no].PC].arguements[1],multi_reg[core_no]);
					if ((address % 4) != 0)
					{
						cout << "ERROR: Invalid memory location " << address << ", give multiples of 4" << endl;
						return 0;
					}
					if (address >= 1048576)
					{
						cout << "ERROR: Memory address " << address << " too large, should be less than 1048576" << endl;
						return 0;
					}
					row_address = (int)(address / 1024);
					col_address = (int)(address % 1024);
					string row_update_info = "";

					

					multi_reg[core_no].cpi++;
					cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; DRAM request issued" << endl;


					struct dram_requests dr;
					dr.inst = "lw";
					dr.register_name = assembly_program_storage[multi_reg[core_no].PC].arguements[0];
					dr.address = address;
					dr.row_address = row_address;
					dr.col_address = col_address;
					dr.dram_request_cpi = multi_reg[core_no].cpi;
					dr.whole_line = assembly_program_storage[multi_reg[core_no].PC].whole_line;

					dram_queue.push_back(dr);

					multi_reg[core_no].PC++;
				}
				else if (assembly_program_storage[multi_reg[core_no].PC].instruction == "sw")
				{

					int time = 0;
					int address = get_address_value(assembly_program_storage[multi_reg[core_no].PC].arguements[1],multi_reg[core_no]);
					if ((address % 4) != 0)
					{
						cout << "ERROR: Invalid memory location " << address << ", give multiples of 4" << endl;
						return 0;
					}
					if (address >= 1048576)
					{
						cout << "ERROR: Memory address " << address << " too large, should be less than 1048576" << endl;
						return 0;
					}
					row_address = (int)(address / 1024);
					col_address = (int)(address % 1024);
					string row_update_info = "";

					

					row_addresses.push_back(row_address);
					col_addresses.push_back(col_address);
					multi_reg[core_no].cpi++;
					cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; DRAM request issued" << endl;

					struct dram_requests dr;
					dr.inst = "sw";
					dr.register_name = assembly_program_storage[multi_reg[core_no].PC].arguements[0];
					dr.address = address;
					dr.row_address = row_address;
					dr.col_address = col_address;
					dr.dram_request_cpi = multi_reg[core_no].cpi;
					dr.whole_line = assembly_program_storage[multi_reg[core_no].PC].whole_line;

					dram_queue.push_back(dr);


					multi_reg[core_no].PC++;
				}
				else if (assembly_program_storage[multi_reg[core_no].PC].instruction == "addi")
				{

					for (int i = 0; i < dram_queue.size(); i++)
					{
						if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[0])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[1])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[2])
						{
							execute_the_queue_rn = true;
						}
					}

					if (grant == time_taken_for_the_printing_of_queue())
					{
						if (grant != 0)
						{
							execute_the_queue_rn = true;
						}
					}
	/*******************************************************Calling reschuling function in every instruction type in following format **********************************************************/
					if (execute_the_queue_rn)
					{
						if (reorder)
						{
							cout << "rescheduling the DRAM queue request" << endl;
							dram_queue = reschedule(dram_queue);
						}
						cout << "executing the queue" << endl;
						string a = print_the_queue(multi_reg[core_no]);
						cout << a;
						execute_the_queue_rn = false;
						grant = 0;
					}

					if (dram_queue.size() > 0)
					{
						grant++;
					}

					if (assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero")
					{
						multi_reg[core_no].cpi++;
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's')
						{
							multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2]);
							cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
						}
						else if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't')
						{
							multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2]);
							cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
						}
					}
					else
					{
						multi_reg[core_no].cpi++;
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2]);
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2]);
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2]);
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2]);
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
					}
					multi_reg[core_no].PC++;
				}
				else if (assembly_program_storage[multi_reg[core_no].PC].instruction == "add")
				{
					for (int i = 0; i < dram_queue.size(); i++)
					{
						if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[0])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[1])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[2])
						{
							execute_the_queue_rn = true;
						}
					}

					if (grant == time_taken_for_the_printing_of_queue())
					{
						if (grant != 0)
						{
							execute_the_queue_rn = true;
						}
					}

					if (execute_the_queue_rn)
					{
						if (reorder)
						{
							cout << "rescheduling the DRAM queue request" << endl;
							dram_queue = reschedule(dram_queue);
						}
						cout << "executing the queue" << endl;
						string a = print_the_queue(multi_reg[core_no]);
						cout << a;
						execute_the_queue_rn = false;
					}

					if (dram_queue.size() > 0)
					{
						grant++;
					}

					multi_reg[core_no].cpi++;
					if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's')
					{
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero")
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0 ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
					}
					else if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't')
					{
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] + multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero")
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0 ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
					}
					multi_reg[core_no].PC++;
				}

				else if (assembly_program_storage[multi_reg[core_no].PC].instruction == "sub")
				{
					for (int i = 0; i < dram_queue.size(); i++)
					{
						if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[0])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[1])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[2])
						{
							execute_the_queue_rn = true;
						}
					}

					if (grant == time_taken_for_the_printing_of_queue())
					{
						if (grant != 0)
						{
							execute_the_queue_rn = true;
						}
					}

					if (execute_the_queue_rn)
					{
						if (reorder)
						{
							cout << "rescheduling the DRAM queue request" << endl;
							dram_queue = reschedule(dram_queue);
						}
						cout << "executing the queue" << endl;
						string a = print_the_queue(multi_reg[core_no]);
						cout << a;
						execute_the_queue_rn = false;
					}

					if (dram_queue.size() > 0)
					{
						grant++;
					}

					multi_reg[core_no].cpi++;
					if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's')
					{
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] - multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] - multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] - multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] - multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero")
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = (-1) * multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = (-1) * multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0 ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
					}
					else if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't')
					{
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] - multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] - multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] - multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] - multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero")
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = (-1) * multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = (-1) * multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0 ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
					}
					multi_reg[core_no].PC++;
				}

				else if (assembly_program_storage[multi_reg[core_no].PC].instruction == "mul")
				{
					for (int i = 0; i < dram_queue.size(); i++)
					{
						if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[0])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[1])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[2])
						{
							execute_the_queue_rn = true;
						}
					}

					if (grant == time_taken_for_the_printing_of_queue())
					{
						if (grant != 0)
						{
							execute_the_queue_rn = true;
						}
					}

					if (execute_the_queue_rn)
					{
						if (reorder)
						{
							cout << "rescheduling the DRAM queue request" << endl;
							dram_queue = reschedule(dram_queue);
						}
						cout << "executing the queue" << endl;
						string a = print_the_queue(multi_reg[core_no]);
						cout << a;
						execute_the_queue_rn = false;
					}

					if (dram_queue.size() > 0)
					{
						grant++;
					}

					multi_reg[core_no].cpi++;
					if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's')
					{
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] * multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] * multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0 ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] * multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] * multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0 ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero")
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0 ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
					}
					else if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't')
					{
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] * multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] * multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0 ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't')
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] * multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] * multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))];
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero")
						{
							if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't')
							{
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
							else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
								multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0 ;
								cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
							}
						}
					}
					multi_reg[core_no].PC++;
				}

				else if(assembly_program_storage[multi_reg[core_no].PC].instruction == "beq"){
					for (int i = 0; i < dram_queue.size(); i++)
					{
						if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[0])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[1])
						{
							execute_the_queue_rn = true;
						}
					}

					if (grant == time_taken_for_the_printing_of_queue())
					{
						if (grant != 0)
						{
							execute_the_queue_rn = true;
						}
					}

					if (execute_the_queue_rn)
					{
						if (reorder)
						{
							cout << "rescheduling the DRAM queue request" << endl;
							dram_queue = reschedule(dram_queue);
						}
						cout << "executing the queue" << endl;
						string a = print_the_queue(multi_reg[core_no]);
						cout << a;
						execute_the_queue_rn = false;
					}

					if (dram_queue.size() > 0)
					{
						grant++;
					}

					multi_reg[core_no].cpi++;
					if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's'){
							if (multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] == multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is not equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't'){
							if (multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] == multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is not equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero"){
							if (multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] == 0){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is not equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
					}
					else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's'){
							if (multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] == multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is not equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't'){
							if (multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] == multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is not equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero"){
							if (multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] == 0){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is not equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}

					}
					else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0] == "$zero"){
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's'){
							if (0 == multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is not equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't'){
							if (0 == multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is not equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero"){
							if (0 == 0){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is not equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
					}
				}


				else if(assembly_program_storage[multi_reg[core_no].PC].instruction == "bne"){
					for (int i = 0; i < dram_queue.size(); i++)
					{
						if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[0])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[1])
						{
							execute_the_queue_rn = true;
						}
					}

					if (grant == time_taken_for_the_printing_of_queue())
					{
						if (grant != 0)
						{
							execute_the_queue_rn = true;
						}
					}

					if (execute_the_queue_rn)
					{
						if (reorder)
						{
							cout << "rescheduling the DRAM queue request" << endl;
							dram_queue = reschedule(dram_queue);
						}
						cout << "executing the queue" << endl;
						string a = print_the_queue(multi_reg[core_no]);
						cout << a;
						execute_the_queue_rn = false;
					}

					if (dram_queue.size() > 0)
					{
						grant++;
					}

					multi_reg[core_no].cpi++;
					if (assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's'){
							if (multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] != multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is not equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't'){
							if (multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] != multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is not equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero"){
							if (multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] != 0){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is not equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
					}
					else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's'){
							if (multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] != multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is not equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't'){
							if (multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] != multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is not equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero"){
							if (multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] != 0){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is not equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}

					}
					else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0] == "$zero"){
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's'){
							if (0 != multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is not equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't'){
							if (0 != multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))]){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is not equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero"){
							if (0 != 0){
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<";value of 1st register is not equal to value of 2nd register, hence jumping to label "<<(assembly_program_storage[multi_reg[core_no].PC].arguements[2])<<endl ;
								multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[2] + ":"];
							}
							else{
								cout << "clock cycle = "<< multi_reg[core_no].cpi<<"; " << assembly_program_storage[multi_reg[core_no].PC].whole_line <<"; value of 1st register is equal to value of 2nd register"<<endl ;
								multi_reg[core_no].PC++;
							}
						}
					}
				}



				else if (assembly_program_storage[multi_reg[core_no].PC].instruction == "slt"){
					for (int i = 0; i < dram_queue.size(); i++)
					{
						if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[0])
						{
							execute_the_queue_rn = true;
						}
						else if (dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[1])
						{
							execute_the_queue_rn = true;
						}
						else if(dram_queue[i].register_name == (string)assembly_program_storage[multi_reg[core_no].PC].arguements[2]){
							execute_the_queue_rn = true;
						}
					}

					if (grant == time_taken_for_the_printing_of_queue())
					{
						if (grant != 0)
						{
							execute_the_queue_rn = true;
						}
					}

					if (execute_the_queue_rn)
					{
						if (reorder)
						{
							cout << "rescheduling the DRAM queue request" << endl;
							dram_queue = reschedule(dram_queue);
						}
						cout << "executing the queue" << endl;
						string a = print_the_queue(multi_reg[core_no]);
						cout << a;
						execute_the_queue_rn = false;
					}

					if (dram_queue.size() > 0)
					{
						grant++;
					}

					multi_reg[core_no].cpi++;

					if (assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 's'){
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's'){
							if (multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] < multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))]){
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
							else{
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't'){
							if (multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] < multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))]){
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
							else{
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
							if (multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] < 0){
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
							else{
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
						}
					}
					else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1][1] == 't'){
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's'){
							if (multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] < multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))]){
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
							else{
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't'){
							if (multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] < multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))]){
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
							else{
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
							if (multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[1].substr(2))] < 0){
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
							else{
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
						}
					}
					else if(assembly_program_storage[multi_reg[core_no].PC].arguements[1] == "$zero"){
						if (assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 's'){
							if (0 < multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))]){
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
							else{
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2][1] == 't'){
							if (0 < multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[2].substr(2))]){
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
							else{
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
						}
						else if(assembly_program_storage[multi_reg[core_no].PC].arguements[2] == "$zero"){
							if (0 < 0){
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 1;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
							else{
								if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 's'){
									multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $s" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].s_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
								else if(assembly_program_storage[multi_reg[core_no].PC].arguements[0][1] == 't'){
									multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] = 0;
									cout << "clock cycle " << multi_reg[core_no].cpi << "; " << assembly_program_storage[multi_reg[core_no].PC].whole_line << "; $t" << stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2)) << "=" << multi_reg[core_no].t_registers[stol(assembly_program_storage[multi_reg[core_no].PC].arguements[0].substr(2))] << endl;
								}
							}
						}
					}
					multi_reg[core_no].PC++;

				}




				else if (assembly_program_storage[multi_reg[core_no].PC].instruction == "j")
				{
					if (grant == time_taken_for_the_printing_of_queue())
					{
						if (grant != 0)
						{
							execute_the_queue_rn = true;
						}
					}

					if (execute_the_queue_rn)
					{
						if (reorder)
						{
							cout << "rescheduling the DRAM queue request" << endl;
							dram_queue = reschedule(dram_queue);
						}
						cout << "executing the queue" << endl;
						string a = print_the_queue(multi_reg[core_no]);
						cout << a;
						execute_the_queue_rn = false;
					}

					if (dram_queue.size() > 0)
					{
						grant++;
					}
					multi_reg[core_no].cpi++;
					cout << "clock cycle " << multi_reg[core_no].cpi << ", jumping to the label " << (assembly_program_storage[multi_reg[core_no].PC].arguements[0]) << endl;
					multi_reg[core_no].PC = label_map[assembly_program_storage[multi_reg[core_no].PC].arguements[0] + ":"];
				}

				else if (label_map.find(assembly_program_storage[multi_reg[core_no].PC].instruction) != label_map.end())
				{
					multi_reg[core_no].PC++;
				}

				else
				{
					cout << "Invalid instruction : " << assembly_program_storage[multi_reg[core_no].PC].instruction;
					for (string a : assembly_program_storage[multi_reg[core_no].PC].arguements)
					{
						cout << " " << a << " ";
					}
					cout << endl;
				}
			}
	
		}
	
	}
	catch (std::exception e)
	{
		std::cout << "Invalid Instruction at line : " << lno[core_no] << '\n';
		cout << "Invalid instruction : " << multi_reg[core_no].assembly_program_storage[multi_reg[core_no].PC].instruction;
		for (string a : multi_reg[core_no].assembly_program_storage[multi_reg[core_no].PC].arguements)
		{
			cout << " " << a << " ";
		}
		cout << endl;
		return 0;
	}

	if (dram_queue.size() > 0)
	{
		if(reorder){
			cout << "rescheduling the DRAM queue request" << endl;
			dram_queue = reschedule(dram_queue);
		}
		cout << "executing the queue" << endl;
		string a = print_the_queue(multi_reg[core_no]);
		cout << a;
	}

	cout << endl;
	cout << "Total execution time in clock cycles = " << multi_reg[core_no].cpi << endl;
	cout << endl;
	cout << "Number of row buffer updates = " << num_of_row_buff_updates << endl;

	cout<<endl;
	cout<<"memory content at the end of the execution-"<<endl;
	for(int i=0;i<1024;i++){
		for(int j=0;j<1024;j++){
			if(dram[i][j]>0){
				int addr=1024*i+j;
				cout<<addr<<"-"<<addr+3<<" : "<<dram[i][j]<<endl;
			}
		}
	}

	for(int i=0;i<NUM_CORES;i++){
		cout <<endl;
		cout<<"final s register values-"<<endl;
		for(int i=0;i<multi_reg[i].s_registers.size();i++){
			cout<<"register $s"<<i<<" : hex=";
			cout<<hex<<multi_reg[i].s_registers[i];
			cout<<", int=";
			cout<<dec<<multi_reg[i].s_registers[i]<<endl;
		}

		cout <<endl;
		cout<<"final t register values-"<<endl;
		for(int i=0;i<multi_reg[i].t_registers.size();i++){
			cout<<"register $t"<<i<<" : hex=";
			cout<<hex<<multi_reg[i].t_registers[i];
			cout<<", int=";
			cout<<dec<<multi_reg[i].t_registers[i]<<endl;
		}
	}	


}
