#include <vector>
#include <math.h>
#include <fstream>
#include <iostream>
#include "matrix.h"
#include <cassert>
#include <algorithm>

// 2D cubic spline

class point
{
public:
    double x;
    double y;
    point()
    {
        x = 0;
        y = 0;
    };
    point(double X, double Y)
    {
        x = X;
        y = Y;
    };
    point& operator+=(const point &B)
    {
        x += B.x;
        y += B.y;
        return *this;
    }
    point& operator-=(const point &B)
    {
        x -= B.x;
        y -= B.y;
        return *this;
    }
};

point operator+(const point &A, const point &B)
{
    point C = A;
    return C += B;
}

point operator-(const point &A, const point &B)
{
    point C = A;
    return C -= B;
}

bool operator<(const point &A, const point &B)
{
    if (A.x < B.x) return true;
    return false;
}

double polynome(std::vector<double> coef, double x)
{
    double y = 0;
    double kx = 1;
    for (int i = 0; i < coef.size(); ++i)
    {
        y += coef[i] * kx;
        kx *= x;
    }
    return y;
}

class curve
{
public:

    void cubic_spline (std::vector<point> &set_points, double dx)
    {
        sort(set_points.begin(), set_points.end());
        int n = set_points.size() - 1;
        assert(n >= 1);
        std::vector<std::vector<double>> table(n + 1, std::vector<double>(n + 1));
        std::vector<double> right_side (n + 1);
        std::vector<double> A (n);
        std::vector<double> B (n);
        std::vector<double> C (n + 1);
        std::vector<double> D (n);
        for (int i = 0; i < n; ++i)
        {
            A[i] = set_points[i].y;
        }
        table[0][0] = 1;
        right_side[0] = 0;
        table[n][n] = 1;
        right_side[n] = 0;
        for (int i = 1; i < n; ++i)
        {
            right_side[i] = (set_points[i+1].y - set_points[i].y) / (set_points[i+1].x - set_points[i].x) 
            - (set_points[i].y - set_points[i - 1].y) / (set_points[i].x - set_points[i - 1].x);
            table[i][i-1] = (set_points[i].x - set_points[i - 1].x) / 3;
            table[i][i] = 2 * (set_points[i + 1].x - set_points[i - 1].x) / 3;
            table[i][i+1] = (set_points[i + 1].x - set_points[i].x) / 3;
        }
        Matrix T(table);
        C = T.TDA(right_side);
        for (int i = 0; i < n; ++i)
        {
            B[i] = (set_points[i+1].y - set_points[i].y) / (set_points[i + 1].x - set_points[i].x) 
            - (C[i + 1] + 2 * C[i]) * (set_points[i+1].x - set_points[i].x) / 3;
            D[i] = (C[i+1] - C[i]) / (3 * (set_points[i+1].x - set_points[i].x));
        }
        for (int i = 0; i < n; ++i)
        {
            std::cout << A[i] << " " << B[i] << " " << C[i] << " " << D[i] << std::endl;
        }
        for (int i = 0; i < n; ++i)
        {
            for (double x = set_points[i].x; x <= set_points[i + 1].x; x += dx)
            {
                double y = polynome({A[i], B[i], C[i], D[i]}, x - set_points[i].x);
                line.push_back({x, y});
            }
        }
    }

    void prn (std::ofstream &output)
    {
        for (auto v: line)
        {
            output << v.x << " " << v.y << std::endl;
        }
    }

    void clearLine()
    {
        line.clear();
    }

    std::vector<point> *getLine()
    {
        return &line;
    }

private:
    std::vector<point> line;
};

int main()
{
    curve graph;
    std::vector<point> initial_set = {{0,0}, {2,2}, {6,3}, {9,8}, {15, 0}};
    graph.cubic_spline(initial_set, 0.1);
    std::ofstream output;
    output.open("out.txt", std::ios_base::out);
    graph.prn(output);
    output.close();
    return 0;
}