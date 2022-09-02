const double eps = 1e-16;
class Matrix
{
public:
    Matrix(std::vector<std::vector<double>> dataset)
    {
        table = dataset;
        n = table.size();
        det = 0;
    };
    std::vector<std::vector<double>> *get_table()
    {
        return &table;
    }
    void prn()
    {
        for (auto &v: table)
        {
            for (auto a: v)
            {
                std::cout << a << " ";
            }
            std::cout << std::endl;
        }
    }
    int get_n()
    {
        return n;
    }
    double get_det()
    {
        return det;
    }
    double find_determinant()
    {
        double ans = 1;
        if (n == 0)
        {
            det = 0;
            return 0;
        }
        auto copy = table;
        int swap_count = 0;
        for (int i = 0; i < n; ++i)
        {
            int max_index = i;
            for (int j = i; j < n; ++j)
            {
                if (fabs(copy[j][i]) > fabs(copy[max_index][i])) max_index = j;
            }
            if (max_index != i)
            {
                std::swap(copy[i], copy[max_index]);
                ++swap_count;
            } 
            if (fabs(copy[i][i]) < eps)
            {
                det = 0;
                return det;
            }
            for (int j = i + 1; j < n; ++j)
            {
                double dev = copy[j][i];
                if (fabs(dev) >= eps)
                {
                    for (int k = i; k < n; ++k)
                    {
                        copy[j][k] -= copy[i][k] * dev / copy[i][i];
                    }
                }
            }
        }
        for (int i = 0; i < n; ++i)
        {
            ans *= copy[i][i];
        }
        if (swap_count % 2) ans *= -1;
        det = ans;
        return ans;
    }
    std::vector<double> Gauss(std::vector<double> right_side)
    {
        auto copy = table;
        std::vector<double> ans (copy.size());
        for (int i = 0; i < n; ++i)
        {
            int max_index = i;
            for (int j = i; j < n; ++j)
            {
                if (fabs(copy[j][i]) > fabs(copy[max_index][i])) max_index = j;
            }
            if (max_index != i)
            {
                std::swap(copy[i], copy[max_index]);
                std::swap(right_side[i], right_side[max_index]);
            }
            if (fabs(copy[i][i]) >= eps)
            {
                for (int j = i; j < n; ++j)
                {
                    double dev = copy[j][i];
                    if (fabs(dev) >= eps)
                    {
                        for (int k = i; k < n; ++k)
                        {
                            copy[j][k] /= dev;
                            if (j!= i) copy[j][k] -= copy[i][k];
                        }
                        right_side[j] /= dev;
                        if (j != i) right_side[j] -= right_side[i];
                    }
                } 
            }
        }
        for (int i = n-1; i >= 0; --i)
        {
            double sum = right_side[i];
            for (int j = i + 1; j < n; ++j)
            {
                sum -= ans[j] * copy[i][j];
            }
            ans[i] = sum;
        }
        return ans;
    }
    std::vector<double> TDA (std::vector<double> right_side) // tridiagonal matrix algorithm
    {
        std::vector<double> A (n);
        std::vector<double> B (n);
        std::vector<double> C (n);
        A[0] = 0;
        B[0] = table[0][0];
        C[0] = table[0][1];
        A[n - 1] = table[n - 1][n - 2];
        B[n - 1] = table[n - 1][n - 1];
        C[n - 1] = 0;
        for (int i = 1; i < n - 1; ++i)
        {
            A[i] = table[i][i - 1];
            B[i] = table[i][i];
            C[i] = table[i][i + 1];
        }

        std::vector<double> alpha (n);
        std::vector<double> beta (n);
        alpha[1] = -C[0] / B[0];
        beta[1] = -right_side[0] / B[0];
        for (int i = 2; i <= n - 1; ++i)
        {
            alpha[i] = -C[i - 1] / (A[i - 1] * alpha[i - 1] + B[i - 1]);
            beta[i] = (right_side[i - 1] - A[i - 1] * beta[i - 1]) / (A[i - 1] * alpha[i - 1] + B[i - 1]);
        }
        std::vector<double> ans (n);
        ans[n - 1] = (right_side[n - 1] - A[n-1] * beta[n - 1]) /(B[n-1] + A[n-1] * alpha[n - 1]);
        for (int i = n - 2; i >= 0; --i)
        {
            ans[i] = alpha[i + 1] * ans[i + 1] + beta[i + 1];
        }
        return ans;
    }
private:
    std::vector<std::vector<double>> table;
    int n;
    double det;
};
