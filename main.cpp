#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <chrono>
using namespace std;
using namespace chrono;

ostream & operator<<(ostream &out, vector<int> &v){
	for (auto &it : v) {
		out << it << " ";
	}
    return out;
}

void PSRS(vector<int> &l, int p);
vector<int> &merge(vector<vector<int>> &ls);

class MyTimer {
private:
	std::chrono::_V2::system_clock::time_point m_start;
	std::chrono::_V2::system_clock::time_point m_end;
public:
	void start() { m_start = std::chrono::_V2::system_clock::now(); }
	void end() { m_end = std::chrono::_V2::system_clock::now(); }
	double get_duration() {
		return (double)std::chrono::duration_cast<std::chrono::microseconds>(m_end - m_start).count()
			* std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
	}
};

class Processor {
private:
	thread th;

public:
	bool par;
	int n_sample;
	vector<int> list;
	vector<int> sample;
	vector<int> elements;
	vector<vector<int>> subs;
	vector<vector<int>> merges;
	vector<int> merged;

	Processor() {}
	void join() {
		if (par) {
			th.join();
			par = false;
		}
	}
	void sort_sample() {
		sort(list.begin(), list.end());
		int j = 0;
		int sample_width = list.size() / n_sample;
		for (int i = 0; i < n_sample; i++, j += sample_width) {
			sample.push_back(list[j]);
		}
	}
	void sort_sample_par() {
		th = thread([=]{
			sort_sample();
		});
		par = true;
	}
	void split() {
		auto it = list.begin();
		for (auto &val : elements) {
			vector<int> sub;
			while (it != list.end() && *it < val) {
				sub.push_back(*it);
				it++;
			}
			subs.push_back(sub);
		}
		vector<int> sub;
		for (; it != list.end(); it++) {
			sub.push_back(*it);
		}
		subs.push_back(sub);
	}
	void split_par() {
		th = thread([=]{
			split();
		});
		par = true;
	}
	void merge() {
		merged = ::merge(merges);
	}
	void merge_par() {
		th = thread([=]{
			merge();
		});
		par = true;
	}
};

void test(int size);

int main(int argc, char const *argv[])
{
	int size[] = {10, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 
	200000, 500000, 1000000, 2000000, 5000000, 10000000, 20000000, 50000000};

	for (int i = 0; i < sizeof(size) / sizeof(size[0]); i++) {
		test(size[i]);
	}

	return 0;
}

void PSRS(vector<int> &l, int p)
{
	bool par = p > 1;
	Processor ps[p];
	int n = l.size();
	int n_p = n / p;

	for (int i = 0; i < p; i++) {
		int begin = n_p * i;
		int end = i == p-1 ? (n) : (begin + n_p);
		ps[i].n_sample = p;
		ps[i].list = vector<int>(l.begin() + begin, l.begin() + end);
	}
	
	int i = 0;
	for (auto &it : ps) {
		if (par)
			it.sort_sample_par();
		else
			it.sort_sample();
	}
	for (auto &it : ps) {
		it.join();
	}

	vector<vector<int>> samples;
	for (auto &it : ps) {
		samples.push_back(it.sample);
	}
	vector<int> ss = merge(samples);
	vector<int> elements;
	for (int i = 0; i < p-1; i++) {
		elements.push_back(ss[p * (i+1)]);
	}

	for (auto &it : ps) {
		it.elements = elements;
		if (par)
			it.split_par();
		else
			it.split();
	}

	for (auto &it : ps) {
		it.join();
	}

	for (int i = 0; i < p; i++) {
		for (int j = 0; j < p; j++) {
			ps[i].merges.push_back(ps[j].subs[i]);
		}
	}

	for (auto &it : ps) {
		if (par)
			it.merge_par();
		else
			it.merge();
	}
	for (auto &it : ps) {
		it.join();
	}

	l.clear();
	for (auto &it : ps) {
		l.insert(l.end(), it.merged.begin(), it.merged.end());
	}
}

vector<int> &merge(vector<vector<int>> &ls)
{
	int num = 0;
	vector<int> cnt;
	for (int i = 0; i < ls.size(); i++) {
		cnt.push_back(0);
	}
	vector<int> *ret = new vector<int>;
	num = 0;

	int end = 0;

	while (!end) {
		end = 1;
		int min, min_idx;
		for (int i = 0; i < ls.size(); i++) {
			if (cnt[i] < ls[i].size()) {
				if (end || ls[i][cnt[i]] < min) {
					min = ls[i][cnt[i]];
					min_idx = i;
					end = 0;
				}
			}
		}
		if (!end) {
			ret->push_back(min);
			cnt[min_idx]++;
		}
	}
	return *ret;
}

void test(int size)
{
	cout << "=============" << endl;
	cout << "Testing size " << size << endl;
	cout << "========" << endl;
	vector<int> list;
	int n = size;

	srand(time(0));
	for (int i = 0; i < n; i++) {
		int x = rand() % (2 * n);
		list.push_back(x);
	}
	vector<int> list2 = list;

	MyTimer t;
	
	t.start();
	sort(list2.begin(), list2.end());
	t.end();
	cout << "sort\t" << t.get_duration() << endl;

	int pnum[4] = {2, 3, 4, 5};
	for (int i = 0; i < 4; i++) {
		vector<int> listt = list;
		t.start();
		PSRS(listt, pnum[i]);
		t.end();
		cout << "PSRS\t" << t.get_duration() << endl;
		int correct = 1;
		for (int i = 0; i < n; i++) {
			if (listt[i] != list2[i]) {
				correct = 0;
				break;
			}
		}
		if (correct) {
			cout << "correct" << endl;
		} else {
			cout << "error" << endl;
		}
	}
	// t.start();
	// PSRS(list, p);
	// t.end();
	// cout << "PSRS\t" << t.get_duration() << endl;
	

	cout << "=============" << endl;
}