#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <boost/algorithm/string.hpp>
using namespace std;


int get_address_value(string s)
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
		ans = s_registers[stol(addr)];
	}
	else
	{
		ans = t_registers[stol(addr)];
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

string print_the_queue()
{
	string answer = "";
	int cpi_number = cpi;
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
					dram[dram_queue[0].row_address][dram_queue[0].col_address] = s_registers[stol(dram_queue[0].register_name.substr(2))];
				}
				else
				{
					dram[dram_queue[0].row_address][dram_queue[0].col_address] = t_registers[stol(dram_queue[0].register_name.substr(2))];
				}
			if (current_row_address == -1)
			{
				//dram[row_address][col_address]=registers[stol(assembly_program_storage[PC].arguements[0].substr(1))];
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
				//dram[row_address][col_address]=registers[stol(assembly_program_storage[PC].arguements[0].substr(1))];
				time += COL_ACCESS_DELAY;
				row_update_info += "; row buffer containg the same row: ";
				row_update_info += to_string(dram_queue[0].row_address);
				row_update_info += ", col offset: ";
				row_update_info += to_string(dram_queue[0].col_address);
				num_of_row_buff_updates += 1;
			}
			else
			{
				//dram[row_address][col_address]=registers[stol(assembly_program_storage[PC].arguements[0].substr(1))];
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
				answer += "clock cycle " + to_string(cpi_number - time + 1) + "-" + to_string(cpi_number) + "; " + dram_queue[0].whole_line + "; memory address " + to_string(dram_queue[0].address) + "-" + to_string(dram_queue[0].address + 3) + "= " + to_string(s_registers[stol(dram_queue[0].register_name.substr(2))]) + row_update_info + "\n";
			}
			else
			{
				answer += "clock cycle " + to_string(cpi_number - time + 1) + "-" + to_string(cpi_number) + "; " + dram_queue[0].whole_line + "; memory address " + to_string(dram_queue[0].address) + "-" + to_string(dram_queue[0].address + 3) + "= " + to_string(t_registers[stol(dram_queue[0].register_name.substr(2))]) + row_update_info + "\n";
			}
		}
		else
		{
			if (dram_queue[0].register_name[1] == 's')
			{
				s_registers[stol(dram_queue[0].register_name.substr(2))] = dram[dram_queue[0].row_address][dram_queue[0].col_address];
			}
			else
			{
				t_registers[stol(dram_queue[0].register_name.substr(2))] = dram[dram_queue[0].row_address][dram_queue[0].col_address];
			}
			if (current_row_address == -1)
			{
				//registers[stol(assembly_program_storage[PC].arguements[0].substr(1))]= dram[row_address][col_address];
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
				//registers[stol(assembly_program_storage[PC].arguements[0].substr(1))]= dram[row_address][col_address];
				time += COL_ACCESS_DELAY;
				row_update_info += "; row buffer containg the same row: ";
				row_update_info += to_string(dram_queue[0].row_address);
				row_update_info += ", col offset: ";
				row_update_info += to_string(dram_queue[0].col_address);
				num_of_row_buff_updates += 0;
			}
			else
			{
				//registers[stol(assembly_program_storage[PC].arguements[0].substr(1))]= dram[row_address][col_address];
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
				answer += "clock cycle " + to_string(cpi_number - time + 1) + "-" + to_string(cpi_number) + "; " + dram_queue[0].whole_line + "; $s" + (dram_queue[0].register_name.substr(2)) + "=" + to_string(s_registers[stol(dram_queue[0].register_name.substr(2))]) + row_update_info + "\n";
			}
			else
			{
				answer += "clock cycle " + to_string(cpi_number - time + 1) + "-" + to_string(cpi_number) + "; " + dram_queue[0].whole_line + "; $t" + (dram_queue[0].register_name.substr(2)) + "=" + to_string(t_registers[stol(dram_queue[0].register_name.substr(2))]) + row_update_info + "\n";
			}
		}
		dram_queue.erase(dram_queue.begin());
	}
	cpi = cpi_number;
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