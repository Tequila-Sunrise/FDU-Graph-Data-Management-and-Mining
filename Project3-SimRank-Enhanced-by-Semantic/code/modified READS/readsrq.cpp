#include "readsrq.h"
#include <math.h>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "timer.h"
#include "sparsehash/sparse_hash_set"
#include "sparsehash/dense_hash_map"
#include "sparsehash/dense_hash_set"
using google::sparse_hash_set;
using google::dense_hash_map;
using google::dense_hash_set;


readsrq::readsrq(char * gName_, int n_, int r_, int rq_, double c_, int t_) {
	sprintf(gName, "%s", gName_);
	n = n_, r = r_, c = c_, t = t_; rq = rq_;
	t1 = t2 = qCnt = 0;

	char iName[125];
	sprintf(iName, "%s.readsrq.%d_%d_%lf_%d", gName, n, r, c, t);


	FILE * fg = fopen(gName, "r");
	if (fg == NULL) {
		printf("No graph %s\n", gName);
		exit(0);
	}
	Timer tm;
	ef.resize(n);
	eb.resize(n);

	for (int x, y; fscanf(fg, "%d%d", &x, &y) != EOF; ) {
		ef[x].push_back(y);
		eb[y].push_back(x);
	}
	rtime = tm.getTime();

	fclose(fg);

	int cc = int(RAND_MAX * c);

	leaf = new vector<array<int, 3> >[r];
	inode = new vector<sparse_hash_map<int, array<int, 3> > >[r];
	sparse_hash_map<int, array<int, 3> >::iterator jt;



	for (int i = 0; i < r; i++) {
		leaf[i].resize(n);
		inode[i].resize(t);

		for (int j = 0; j < t; j++)
			inode[i][j].set_deleted_key(-1);

		for (int j = 0, p; j < n; j++)
			if (!eb[j].empty()) {
				leaf[i][j] = {p = eb[j][rand() % eb[j].size()], -1, -1};
				if ((jt = inode[i][0].find(p)) == inode[i][0].end())
					inode[i][0][p] = { -1, j, j};
				else {
					leaf[i][j][1] = jt->second[2];
					leaf[i][jt->second[2]][2] = j;
					inode[i][0][p][2] = j;
				}
			} else leaf[i][j] = { -1, -1, -1};


		for (int j = 0, rr; j < t - 1 && inode[i][j].size() > 1; j++)
			for (auto it = inode[i][j].begin(); it != inode[i][j].end(); it++)
				if (!eb[it->first].empty() && ( (rr = rand()) < cc || j == 0 ) ) {
					it->second[0] = eb[it->first][rr % eb[it->first].size()];
					if ((jt = inode[i][j + 1].find(it->second[0])) == inode[i][j + 1].end())
						inode[i][j + 1][it->second[0]] = { -1, it->second[1], it->second[2]};
					else {
						leaf[i][it->second[1]][1] = jt->second[2];
						leaf[i][jt->second[2]][2] = it->second[1];
						jt->second[2] = it->second[2];
					}
				}



	}

	for (int i = 0; i < n; i++) {
		random_shuffle(ef[i].begin(), ef[i].end());
		random_shuffle(eb[i].begin(), eb[i].end());
	}


}


readsrq::~readsrq() {
	delete [] leaf;
	delete [] inode;
}


void readsrq::queryAll(int x, double * ansVal) {

	memset(ansVal, 0, sizeof(double)*n);
	if (eb[x].empty()) {
		ansVal[x] = 1;
		return;
	}

	if (qCnt >= 20 && t1 < t2 || qCnt < 20 && qCnt % 2 == 0) {
		if (qCnt < 20) tm.reset();

		dense_hash_map<int, double> sim1;
		sim1.set_empty_key(-1);

		double cc = (1 - c) * r * rq / eb[x].size(), c1 = rq * r / c / eb[x].size();


		int p;
		for (auto i = eb[x].begin(); i != eb[x].end(); i++) {
			int q;
			for (auto j = eb[p = *i].begin(); j != eb[p].end() && j - eb[p].begin() < 10; j++)
				for (auto k = ef[q = *j].begin(); k != ef[q].end() && k - ef[q].begin() < 10; k++)
					if (*k != p)
						sim1[*k] += cc / eb[p].size() / eb[*k].size();
			sim1[*i] += c1;
		}


		sparse_hash_map<int, array<int, 3> >::iterator inodeIT;
		vector<int> q(t);


		for (int i = 0, ncnt = 0; i < r; i++)
			for (int ir = 0, k; ir < rq; ir++) {
				int j = eb[x][(ncnt++) % eb[x].size()];

				if (eb[j].empty()) continue;

				for (q[k = 0] = eb[j][rand() % eb[j].size()]; !eb[q[k]].empty() && k < t - 1; k++) {
					if ((inodeIT = inode[i][k].find(q[k])) != inode[i][k].end()) {
						if (inodeIT->second[0] != -1) q[k + 1] = inodeIT->second[0];
						else {
							q[k + 1] = eb[q[k]][rand() % eb[q[k]].size()];
							q[k] -= n;
						}
					} else q[k + 1] = eb[q[k]][rand() % eb[q[k]].size()];
				}

				while (k >= 0 && q[k] >= 0 && inode[i][k].find(q[k]) == inode[i][k].end()) k--;
				if (k < 0) continue;
				if (q[k] >= 0) q[k] -= n;

				for (; k >= 0; k--)
					if (q[k] < 0) {
						q[k] += n;
						for (int jj = inode[i][k][q[k]][1]; jj != -1; jj = leaf[i][jj][2])
							if (jj != j) sim1[jj] += c;
					}


			}



		cc = c * c / r / rq;
		for (auto it = sim1.begin(); it != sim1.end(); it++)
			for (auto jt = ef[p = it->first].begin(); jt != ef[p].end(); jt++)
				ansVal[*jt] += it->second * cc / eb[*jt].size();


		if (qCnt++ < 20) t1 += tm.getTime();
	} else {

		if (qCnt < 20) tm.reset();

		double * sim1 = new double[n];
		memset(sim1, 0, sizeof(double)*n);

		//sim1, sim2, ansVal = real*g[x][0]*r/c^2

		double cc = (1 - c) * r * rq / eb[x].size(), c1 = rq * r / c / eb[x].size();



		int p;
		for (auto i = eb[x].begin(); i != eb[x].end(); i++) {
			int q;
			for (auto j = eb[p = *i].begin(); j != eb[p].end() && j - eb[p].begin() < 10; j++)
				for (auto k = ef[q = *j].begin(); k != ef[q].end() && k - ef[q].begin() < 10; k++)
					if (*k != p)
						sim1[*k] += cc / eb[p].size() / eb[*k].size();
			sim1[*i] += c1;
		}


		sparse_hash_map<int, array<int, 3> >::iterator inodeIT;
		vector<int> q(t);


		for (int i = 0, ncnt = 0; i < r; i++)
			for (int ir = 0, k; ir < rq; ir++) {
				int j = eb[x][(ncnt++) % eb[x].size()];

				if (eb[j].empty()) continue;

				for (q[k = 0] = eb[j][rand() % eb[j].size()]; !eb[q[k]].empty() && k < t - 1; k++) {
					if ((inodeIT = inode[i][k].find(q[k])) != inode[i][k].end()) {
						if (inodeIT->second[0] != -1) q[k + 1] = inodeIT->second[0];
						else {
							q[k + 1] = eb[q[k]][rand() % eb[q[k]].size()];
							q[k] -= n;
						}
					} else q[k + 1] = eb[q[k]][rand() % eb[q[k]].size()];
				}

				while (k >= 0 && q[k] >= 0 && inode[i][k].find(q[k]) == inode[i][k].end()) k--;
				if (k < 0) continue;
				if (q[k] >= 0) q[k] -= n;

				for (; k >= 0; k--)
					if (q[k] < 0) {
						q[k] += n;
						for (int jj = inode[i][k][q[k]][1]; jj != -1; jj = leaf[i][jj][2])
							if (jj != j) sim1[jj] += c;
					}


			}





		for (int i = 0; i < n; i++)
			if (sim1[i] > 0)
				for (auto j = ef[i].begin(); j != ef[i].end(); j++)
					ansVal[*j] += sim1[i];
		cc = c * c / r / rq;
		for (int i = 0; i < n; i++)
			if (eb[i].size() > 0)
				ansVal[i] = cc * ansVal[i] / eb[i].size();

		if (qCnt++ < 20) t2 += tm.getTime();

	}

}




void readsrq::insEdge(int x, int y) {

	ef[x].push_back(y);
	eb[y].push_back(x);

	sparse_hash_map<int, array<int, 3> >::iterator it, kt;
	array<int, 3> * jt;
	int cc = int(RAND_MAX * c);


	for (int i = 0, o1, o2, rr; i < r; i++) {
		if ( (jt = &leaf[i][y])->at(0) == -1 || rand() < RAND_MAX / eb[y].size() && jt->at(0) != x) {

			if (jt->at(0) != -1) {

				for (int j = 0, p = jt->at(0); j < t && p != -1; j++)
					if ( (it = inode[i][j].find(p))->second[1] == y ) {
						p = it->second[0];
						if (it->second[2] == y) inode[i][j].erase(it);
						else it->second[1] = jt->at(2);
					} else if (it->second[2] == y) {
						p = it->second[0];
						it->second[2] = jt->at(1);
					} else break;

				if (jt->at(1) != -1)
					leaf[i][jt->at(1)][2] = jt->at(2);
				if (jt->at(2) != -1)
					leaf[i][jt->at(2)][1] = jt->at(1);
			}

			if ( (it = inode[i][0].find(x)) != inode[i][0].end()) {

				jt->at(0) = x;
				if ((jt->at(2) = leaf[i][it->second[2]][2]) != -1)
					leaf[i][ jt->at(2) ][1] = y;
				if ((jt->at(1) = it->second[2]) != -1)
					leaf[i][ jt->at(1) ][2] = y;

				for (int j = 0, p = x; j < t && p != -1; j++) {
					if ( (it = inode[i][j].find(p))->second[2] == jt->at(1) ) it->second[2] = y;
					else break;
					p = it->second[0];
				}
			} else {

				*jt = {x, -1, -1};
				int j = 0, p = x;

				for (int q; j < t - 1; j++) {
					if (eb[p].empty() || (rr = rand()) >= cc && j != 0 ) {
						inode[i][j][p] = { -1, y, y};
						break;
					} else {
						inode[i][j][p] = {q = eb[p][rr % eb[p].size()], y, y};
						p = q;
						if ( (it = inode[i][j + 1].find(p)) != inode[i][j + 1].end() ) {

							if ((jt->at(2) = leaf[i][it->second[2]][2]) != -1)
								leaf[i][ jt->at(2) ][1] = y;
							if ((jt->at(1) = it->second[2]) != -1)
								leaf[i][ jt->at(1) ][2] = y;

							for (j++; j < t && p != -1; j++) {
								if ( (it = inode[i][j].find(p))->second[2] == jt->at(1) ) it->second[2] = y;
								else break;
								p = it->second[0];
							}
							j = t - 2;
							break;
						}
					}
				}

				if (j == t - 1) {
					inode[i][j][p] = { -1, y, y};
				}
			}

		}


		for (int j = 0; j < t - 1; j++) {

			if ( (kt = inode[i][j].find(y)) != inode[i][j].end() &&
			     (j == 0 && kt->second[0] == -1 || kt->second[0] != -1 && kt->second[0] != x && rand() < RAND_MAX / eb[y].size()) ) {



				if (kt->second[0] != -1) {

					o1 = leaf[i][kt->second[1]][1], o2 = leaf[i][kt->second[2]][2];
					for (int k = j + 1, p = kt->second[0]; j < t && p != -1; k++) {
						if ( (it = inode[i][k].find(p))->second[1] == kt->second[1] ) {
							p = it->second[0];
							if (it->second[2] == kt->second[2]) inode[i][k].erase(it);
							else {
								it->second[1] = o2;
							}
						} else if (it->second[2] == kt->second[2]) {
							p = it->second[0];
							it->second[2] = o1;
						} else break;
					}

					if (o1 != -1) leaf[i][o1][2] = o2;
					if (o2 != -1) leaf[i][o2][1] = o1;

				}

				if ( (it = inode[i][j + 1].find(x)) != inode[i][j + 1].end() ) {

					kt->second[0] = x;
					if ( (o2 = leaf[i][kt->second[2]][2] = leaf[i][it->second[2]][2]) != -1 )
						leaf[i][o2][1] = kt->second[2];
					if ( (o1 = leaf[i][kt->second[1]][1] = it->second[2]) != -1)
						leaf[i][o1][2] = kt->second[1];

					for (int k = j + 1, p = x; k < t && p != -1; k++) {
						if ( (it = inode[i][k].find(p))->second[2] == o1) it->second[2] = kt->second[2];
						else break;
						p = it->second[0];
					}

				} else {

					kt->second[0] = x;
					leaf[i][kt->second[1]][1] = leaf[i][kt->second[2]][2] = -1;
					int k = j + 1, p = x;
					for (int q; k < t - 1; k++) {
						if (eb[p].empty() || (rr = rand()) >= cc) {
							inode[i][k][p] = { -1, kt->second[1], kt->second[2]};
							break;
						} else {
							inode[i][k][p] = {q = eb[p][rr % eb[p].size()], kt->second[1], kt->second[2]};
							p = q;
							if ( (it = inode[i][k + 1].find(p)) != inode[i][k + 1].end() ) {
								if ( (o2 = leaf[i][kt->second[2]][2] = leaf[i][it->second[2]][2]) != -1 )
									leaf[i][o2][1] = kt->second[2];
								if ( (o1 = leaf[i][kt->second[1]][1] = it->second[2]) != -1)
									leaf[i][o1][2] = kt->second[1];


								for (k++; k < t && p != -1; k++) {
									if ( (it = inode[i][k].find(p))->second[2] == o1 ) it->second[2] = kt->second[2];
									else break;
									p = it->second[0];
								}
								k = t - 2;
								break;
							}

						}

					}

					if (k == t - 1) {
						inode[i][k][p] = { -1, kt->second[1], kt->second[2]};
					}
				}


			}




		}



	}







}








void readsrq::delEdge(int x, int y) {
	bool fy = 0;
	for (auto it = ef[x].begin(); it != ef[x].end(); it++)
		if (*it == y) {
			ef[x].erase(it);
			fy = 1;
			break;
		}
	if (!fy) return;
	for (auto it = eb[y].begin(); it != eb[y].end(); it++)
		if (*it == x) {
			eb[y].erase(it);
			break;
		}


	sparse_hash_map<int, array<int, 3> >::iterator it, kt;
	array<int, 3> * jt;
	int cc = int(RAND_MAX * c);


	for (int i = 0, o1, o2, rr, z; i < r; i++) {

		if ( (jt = &leaf[i][y])->at(0) == x) {

			for (int j = 0, p = jt->at(0); j < t && p != -1; j++)
				if ( (it = inode[i][j].find(p))->second[1] == y ) {
					p = it->second[0];
					if (it->second[2] == y) inode[i][j].erase(it);
					else it->second[1] = jt->at(2);
				} else if (it->second[2] == y) {
					p = it->second[0];
					it->second[2] = jt->at(1);
				} else break;

			if (jt->at(1) != -1)
				leaf[i][jt->at(1)][2] = jt->at(2);
			if (jt->at(2) != -1)
				leaf[i][jt->at(2)][1] = jt->at(1);

			if (eb[y].empty()) {
				*jt = { -1, -1, -1};
				continue;
			} else z = eb[y][rand() % eb[y].size()];

			if ( (it = inode[i][0].find(z)) != inode[i][0].end()) {
				jt->at(0) = z;
				if ((jt->at(2) = leaf[i][it->second[2]][2]) != -1)
					leaf[i][ jt->at(2) ][1] = y;
				if ((jt->at(1) = it->second[2]) != -1)
					leaf[i][ jt->at(1) ][2] = y;

				for (int j = 0, p = z; j < t && p != -1; j++) {
					if ( (it = inode[i][j].find(p))->second[2] == jt->at(1) ) it->second[2] = y;
					else break;
					p = it->second[0];
				}
			} else {

				*jt = {z, -1, -1};
				int j = 0, p = z;

				for (int q; j < t - 1; j++) {
					if (eb[p].empty() || (rr = rand()) >= cc && j != 0 ) {
						inode[i][j][p] = { -1, y, y};
						break;
					} else {
						inode[i][j][p] = {q = eb[p][rr % eb[p].size()], y, y};
						p = q;
						if ( (it = inode[i][j + 1].find(p)) != inode[i][j + 1].end() ) {

							if ((jt->at(2) = leaf[i][it->second[2]][2]) != -1)
								leaf[i][ jt->at(2) ][1] = y;
							if ((jt->at(1) = it->second[2]) != -1)
								leaf[i][ jt->at(1) ][2] = y;

							for (j++; j < t && p != -1; j++) {
								if ( (it = inode[i][j].find(p))->second[2] == jt->at(1) ) it->second[2] = y;
								else break;
								p = it->second[0];
							}
							j = t - 2;
							break;
						}
					}
				}


				if (j == t - 1) {
					inode[i][j][p] = { -1, y, y};
				}
			}

		}


		for (int j = 0; j < t - 1; j++) {

			if ( (kt = inode[i][j].find(y)) != inode[i][j].end() && kt->second[0] == x ) {

				o1 = leaf[i][kt->second[1]][1], o2 = leaf[i][kt->second[2]][2];
				for (int k = j + 1, p = kt->second[0]; j < t && p != -1; k++) {
					if ( (it = inode[i][k].find(p))->second[1] == kt->second[1] ) {
						p = it->second[0];
						if (it->second[2] == kt->second[2]) inode[i][k].erase(it);
						else {
							it->second[1] = o2;
						}
					} else if (it->second[2] == kt->second[2]) {
						p = it->second[0];
						it->second[2] = o1;
					} else break;
				}

				if (o1 != -1) leaf[i][o1][2] = o2;
				if (o2 != -1) leaf[i][o2][1] = o1;

				if (eb[y].empty()) {
					kt->second[0] = -1;
					leaf[i][kt->second[1]][1] = leaf[i][kt->second[2]][2] = -1;
					continue;
				} else z = eb[y][rand() % eb[y].size()];


				if ( (it = inode[i][j + 1].find(z)) != inode[i][j + 1].end() ) {

					kt->second[0] = z;
					if ( (o2 = leaf[i][kt->second[2]][2] = leaf[i][it->second[2]][2]) != -1 )
						leaf[i][o2][1] = kt->second[2];
					if ( (o1 = leaf[i][kt->second[1]][1] = it->second[2]) != -1)
						leaf[i][o1][2] = kt->second[1];

					for (int k = j + 1, p = z; k < t && p != -1; k++) {
						if ( (it = inode[i][k].find(p))->second[2] == o1) it->second[2] = kt->second[2];
						else break;
						p = it->second[0];
					}

				} else {

					kt->second[0] = z;
					leaf[i][kt->second[1]][1] = leaf[i][kt->second[2]][2] = -1;
					int k = j + 1, p = z;
					for (int q; k < t - 1; k++) {
						if (eb[p].empty() || (rr = rand()) >= cc) {
							inode[i][k][p] = { -1, kt->second[1], kt->second[2]};
							break;
						} else {
							inode[i][k][p] = {q = eb[p][rr % eb[p].size()], kt->second[1], kt->second[2]};
							p = q;
							if ( (it = inode[i][k + 1].find(p)) != inode[i][k + 1].end() ) {
								if ( (o2 = leaf[i][kt->second[2]][2] = leaf[i][it->second[2]][2]) != -1 )
									leaf[i][o2][1] = kt->second[2];
								if ( (o1 = leaf[i][kt->second[1]][1] = it->second[2]) != -1)
									leaf[i][o1][2] = kt->second[1];

								for (k++; k < t && p != -1; k++) {
									if ( (it = inode[i][k].find(p))->second[2] == o1 ) it->second[2] = kt->second[2];
									else break;
									p = it->second[0];
								}
								k = t - 2;
								break;
							}

						}

					}

					if (k == t - 1) {
						inode[i][k][p] = { -1, kt->second[1], kt->second[2]};
					}
				}


			}




		}




	}







}