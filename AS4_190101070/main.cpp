#include <stdio.h>
#include <iostream>
#include <fstream>
#include <conio.h>
#include <vector>
#include <bits/stdc++.h>

using namespace std;

string ROLLNUM = "190101070_";
string RECORDING_DIR = "recordings/";

vector<vector<float>>
read_file(string letter)
{
	vector<vector<float>> a;
	float num;
	vector<float> buf;

	ifstream rec;

	string file_prefix = RECORDING_DIR + ROLLNUM + letter + "_";

	for (int i = 1; i <= 10; i++)
	{
		rec.open(file_prefix + to_string(i) + ".txt");
		cout << "Reading " << file_prefix << to_string(i) << ".txt" << endl;
		while (rec)
		{
			rec >> num;
			buf.push_back(num);
		}
		a.push_back(buf);
		buf.clear();
		rec.close();
	}

	return a;
}

vector<vector<float>> read_file_test(string letter)
{
	vector<vector<float>> a;
	float num;
	vector<float> buf;

	ifstream rec;

	string file_prefix = RECORDING_DIR + ROLLNUM + letter + "_";

	for (int i = 11; i <= 20; i++)
	{
		rec.open(file_prefix + to_string(i) + ".txt");
		cout << "Reading " << file_prefix << to_string(i) << ".txt" << endl;
		while (rec)
		{
			rec >> num;
			buf.push_back(num);
		}
		a.push_back(buf);
		buf.clear();
		rec.close();
	}

	return a;
}

vector<float> calculate_dc_shift(vector<vector<float>> a)
{
	vector<float> dc_shift;
	float sum = 0;
	vector<float> x;

	for (int i = 0; i < 10; i++)
	{
		x = a[i];
		for (int j = 0; j < 100; j++)
		{
			sum += x[j];
		}
		dc_shift.push_back(sum / 100);
		// cout << sum / 100 << endl;
		sum = 0;
		x.clear();
	}
	return dc_shift;
}

vector<vector<float>> normalize(vector<vector<float>> recordings, vector<float> dc_shift)
{
	vector<vector<float>> normalized;
	for (int k = 0; k < 10; k++)
	{
		vector<float> a = recordings[k];
		float shift = dc_shift[k];
		float m = INT_MIN, mn = INT_MAX;
		int n = a.size();

		for (int i = 0; i < n; i++)
		{
			a[i] = a[i] - shift;
			if (a[i] > m)
				m = a[i];
			if (a[i] < mn)
				mn = a[i];
		}

		// cout << "Max " << m << " Min " << mn << endl;
		float abs_max = max(abs(m), abs(mn));
		// cout << "Max " << m << " Min " << mn << " Abs Max " << abs_max << endl;

		for (int i = 0; i < n; i++)
		{
			a[i] = a[i] * 5000 / abs_max;
		}

		normalized.push_back(a);
	}
	return normalized;
}

vector<vector<vector<float>>> get_steady_state(vector<vector<float>> a)
{
	// SELECT 5 Frames from Steady State
	vector<vector<vector<float>>> frames(10, vector<vector<float>>(5, vector<float>(320, 0)));

	int count = 0, ix = 5000;
	for (int j = 0; j < 10; j++)
	{
		while (ix < 6600)
		{
			frames[j][count / 320][count % 320] = a[j][ix] * (0.54 - 0.46 * cos((2 * 3.14159 * (count % 320)) / 319));
			ix++;
			count++;
		}
		ix = 5000;
		count = 0;
	}

	return frames;
}

vector<vector<vector<float>>> calculate_R(vector<vector<vector<float>>> frames)
{
	// 10 x 5 x 13
	vector<vector<vector<float>>> R(
		10, vector<vector<float>>(
				5, vector<float>(
					   13, 0)));

	float temp = 0;
	for (int m = 0; m < 10; m++)
	{
		for (int x = 0; x < 5; x++)
		{
			for (int i = 0; i < 13; i++)
			{
				for (int j = 0; j < 320 - i; j++)
					temp += frames[m][x][j] * frames[m][x][j + i];
				// cout << temp << " ";
				R[m][x][i] = temp;
				temp = 0;
			}
			// cout << endl;
		}
		// cout << endl;
	}

	return R;
}

vector<vector<float>> calculate_distance(vector<vector<float>> Ci, vector<vector<float>> C_final)
{
	// Ci = 5 x 12
	// C_final = 10 x 12

	vector<vector<float>> D(10, vector<float>(5, 0));
	vector<float> w = {1.0, 3.0, 7.0, 13.0, 19.0, 22.0, 25.0, 33.0, 42.0, 50.0, 56.0, 61.0};
	float sum = 0;
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			for (int m = 0; m < 12; m++)
			{
				sum += ((Ci[j][m] - C_final[i][m]) * (Ci[j][m] - C_final[i][m])) / w[m];
			}
			D[i][j] = sum;
			sum = 0;
		}
	}
	// for (int i = 0; i < 10; i++)
	// {
	// 	for (int j = 0; j < 5; j++)
	// 	{
	// 		cout << D[i][j] << endl;
	// 	}
	// }

	return D;
}

int main()
{
	vector<string> vowels = {"a", "e", "i", "o", "u"};
	vector<vector<float>> Ci;

	for (int z = 0; z < 5; z++)
	{

		// Training
		vector<vector<float>> recordings;
		recordings = read_file(vowels[z]);

		vector<float> dc_shift = calculate_dc_shift(recordings);

		vector<vector<float>> normalized = normalize(recordings, dc_shift);

		vector<vector<vector<float>>> frames = get_steady_state(normalized);

		vector<vector<vector<float>>> R = calculate_R(frames);

		// 10 x 5 x 13
		vector<vector<vector<float>>> E(
			10, vector<vector<float>>(
					5, vector<float>(13, 0)));

		for (int m = 0; m < 10; m++)
		{
			for (int x = 0; x < 5; x++)
			{
				E[m][x][0] = R[m][x][0];
				E[m][x][1] = (R[m][x][0] * R[m][x][0] - R[m][x][1] * R[m][x][1]) / R[m][x][0];
			}
		}

		vector<vector<vector<float>>> k(
			10, vector<vector<float>>(
					5, vector<float>(13, 0)));
		float sum = 0;

		vector<vector<vector<vector<float>>>> alpha(
			10, vector<vector<vector<float>>>(
					5, vector<vector<float>>(
						   13, vector<float>(13, 0))));
		for (int m = 0; m < 10; m++)
		{
			for (int x = 0; x < 5; x++)
			{
				k[m][x][1] = R[m][x][1] / R[m][x][0];
				alpha[m][x][0][0] = 0;
				alpha[m][x][1][1] = k[m][x][1];
			}
		}

		vector<vector<vector<float>>> A(
			10, vector<vector<float>>(
					5, vector<float>(13, 0)));
		for (int m = 0; m < 10; m++)
		{
			for (int x = 0; x < 5; x++)
			{
				for (int i = 2; i <= 12; i++)
				{
					for (int j = 1; j <= i - 1; j++)
					{
						sum += alpha[m][x][j][i - 1] * R[m][x][i - j];
					}

					k[m][x][i] = (R[m][x][i] - sum) / E[m][x][i - 1];
					sum = 0;
					alpha[m][x][i][i] = k[m][x][i];
					for (int j = 1; j <= i - 1; j++)
					{
						alpha[m][x][j][i] = alpha[m][x][j][i - 1] - k[m][x][i] * alpha[m][x][i - j][i - 1];
					}
					E[m][x][i] = (1 - k[m][x][i] * k[m][x][i]) * E[m][x][i - 1];
				}
			}
		}

		for (int m = 0; m < 10; m++)
		{
			for (int x = 0; x < 5; x++)
			{
				for (int i = 1; i <= 12; i++)
				{
					A[m][x][i] = alpha[m][x][i][12];
					// cout << A[m][x][i] << " ";
				}
				// cout << endl;
			}
			// cout << endl;
		}

		vector<vector<vector<float>>> C(
			10, vector<vector<float>>(
					5, vector<float>(13, 0)));

		for (int m = 0; m < 10; m++)
		{
			for (int x = 0; x < 5; x++)
			{
				C[m][x][0] = log(R[m][x][0]);
				C[m][x][1] = A[m][x][1];
			}
		}

		sum = 0;
		for (int i = 0; i < 10; i++)
		{
			for (int x = 0; x < 5; x++)
			{
				for (int m = 2; m <= 12; m++)
				{
					for (int j = 1; j <= m - 1; j++)
					{
						sum += (j * C[i][x][j] * A[i][x][m - j]) / m;
					}
					C[i][x][m] = A[i][x][m] + sum;

					sum = 0;
				}
			}
		}

		sum = 0;
		vector<float> C_final(12, 0);
		for (int m = 1; m <= 12; m++)
		{
			for (int i = 0; i < 10; i++)
			{
				for (int x = 0; x < 5; x++)
				{

					sum += C[i][x][m];
				}
			}
			C_final[m] = sum / 50;
			cout << "C" << m + 1 << " " << C_final[m] << endl;

			sum = 0;
		}
		Ci.push_back(C_final);
	}
	// cout << "Training Done" << endl;
	// for (int i = 0; i < Ci.size(); i++)
	// {
	// 	for (int j = 0; j < Ci[i].size(); j++)
	// 	{
	// 		cout << Ci[i][j] << " ";
	// 	}
	// 	cout << endl;
	// }

	for (int z = 0; z < 5; z++)
	{

		// Testing
		vector<vector<float>> recordings_test = read_file_test(vowels[z]);
		vector<float> dc_shift_test = calculate_dc_shift(recordings_test);
		vector<vector<float>> normalized_test = normalize(recordings_test, dc_shift_test);
		vector<vector<vector<float>>> frames_test = get_steady_state(normalized_test);

		vector<vector<vector<float>>> R_test = calculate_R(frames_test);

		vector<vector<vector<float>>> E_test(
			10, vector<vector<float>>(
					5, vector<float>(13, 0)));

		for (int m = 0; m < 10; m++)
		{
			for (int x = 0; x < 5; x++)
			{
				E_test[m][x][0] = R_test[m][x][0];
				E_test[m][x][1] = (R_test[m][x][0] * R_test[m][x][0] - R_test[m][x][1] * R_test[m][x][1]) / R_test[m][x][0];
			}
		}

		vector<vector<vector<float>>> k_test(
			10, vector<vector<float>>(
					5, vector<float>(13, 0)));
		float sum = 0;

		vector<vector<vector<vector<float>>>> alpha_test(
			10, vector<vector<vector<float>>>(
					5, vector<vector<float>>(
						   13, vector<float>(13, 0))));
		for (int m = 0; m < 10; m++)
		{
			for (int x = 0; x < 5; x++)
			{
				k_test[m][x][1] = R_test[m][x][1] / R_test[m][x][0];
				alpha_test[m][x][0][0] = 0;
				alpha_test[m][x][1][1] = k_test[m][x][1];
			}
		}

		vector<vector<vector<float>>> A_test(
			10, vector<vector<float>>(
					5, vector<float>(13, 0)));
		for (int m = 0; m < 10; m++)
		{
			for (int x = 0; x < 5; x++)
			{
				for (int i = 2; i <= 12; i++)
				{
					for (int j = 1; j <= i - 1; j++)
					{
						sum += alpha_test[m][x][j][i - 1] * R_test[m][x][i - j];
					}

					k_test[m][x][i] = (R_test[m][x][i] - sum) / E_test[m][x][i - 1];
					sum = 0;
					alpha_test[m][x][i][i] = k_test[m][x][i];
					for (int j = 1; j <= i - 1; j++)
					{
						alpha_test[m][x][j][i] = alpha_test[m][x][j][i - 1] - k_test[m][x][i] * alpha_test[m][x][i - j][i - 1];
					}
					E_test[m][x][i] = (1 - k_test[m][x][i] * k_test[m][x][i]) * E_test[m][x][i - 1];
				}
			}
		}

		for (int m = 0; m < 10; m++)
		{
			for (int x = 0; x < 5; x++)
			{
				for (int i = 1; i <= 12; i++)
				{
					A_test[m][x][i] = alpha_test[m][x][i][12];
					// cout << A[m][x][i] << " ";
				}
				// cout << endl;
			}
			// cout << endl;
		}

		vector<vector<vector<float>>> C_test(
			10, vector<vector<float>>(
					5, vector<float>(13, 0)));

		for (int m = 0; m < 10; m++)
		{
			for (int x = 0; x < 5; x++)
			{
				C_test[m][x][0] = log(R_test[m][x][0]);
				C_test[m][x][1] = A_test[m][x][1];
			}
		}

		sum = 0;
		for (int i = 0; i < 10; i++)
		{
			for (int x = 0; x < 5; x++)
			{
				for (int m = 2; m <= 12; m++)
				{
					for (int j = 1; j <= m - 1; j++)
					{
						sum += (j * C_test[i][x][j] * A_test[i][x][m - j]) / m;
					}
					C_test[i][x][m] = A_test[i][x][m] + sum;

					sum = 0;
				}
			}
		}

		sum = 0;

		vector<vector<float>> C_final_test(10, vector<float>(12, 0));
		for (int m = 1; m <= 12; m++)
		{
			for (int i = 0; i < 10; i++)
			{
				for (int x = 0; x < 5; x++)
				{
					sum += C_test[i][x][m];
				}
				C_final_test[i][m] = sum / 5;
				sum = 0;
				// cout << C_final_test[i][m] << " ";

				cout << C_final_test[i][m] << endl;
			}
			// cout << endl;
		}

		// cout << Ci.size() << " x " << Ci[0].size() << endl;

		vector<vector<float>> D = calculate_distance(Ci, C_final_test);
		int prediction = -1;
		float minDist = INT_MAX;
		for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 5; j++)
			{
				if (minDist > D[i][j])
				{
					minDist = D[i][j];
					prediction = j;
				}
			}
		}
	}

	// _getch();
	return 0;
}