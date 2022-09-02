#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <fstream>
#include <ios>
#include "matrix.h"

class SystemState
{
public:
    SystemState(const std::vector<double> &initial_conditions, const std::vector<double> &coef, double start_x, double dx): 
    state(initial_conditions), coefficients(coef), h(dx), x(start_x)
    {
        assert(coefficients.size() == state.size() + 2); // - число коэффициентов должно быть на 2 больше числа начальных условий
        assert(fabs(coefficients.back()) > eps); // - коэффициент при высшей степени не должен быть нулевым
        n = state.size();
    };
    void calculateNextIt () // - пересчет следующего состояния системы по методу Эйлера
    {
        std::vector<double> new_state(n);
        for (int i = 0; i < n - 1; ++i)
        {
            new_state[i] = state[i] + state[i+1] * h;
        }
        new_state.back() = state.back() - h*coefficients[0]/coefficients.back(); // - добавление свободного члена
        for (int i = 0; i < n; ++i)
        {
            new_state.back() += -h*coefficients[i + 1] * state[i]/coefficients.back();
        }
        state = new_state;
        x += h;
    }
    std::vector<double> getState ()
    {
        return state;
    }
    double getX()
    {
        return x;
    }
private:
    std::vector<double> state;
    std::vector<double> coefficients;
    double x;
    double h;
    int n;
};
std::ostream& operator << (std::ostream &out, const std::vector<double> &a)
{
    for (auto v: a)
    {
        out << v << " ";
    }
    return out;
} 
int main()
{
    double h = 0.01;
    SystemState p({0, 3, -9, -8, 0}, {0, 243, 405, 270, 90, 15, 1},  0, h); // - задание коэффициентов, начальных условий задачи Коши*/
    std::ofstream output;
    output.open("graph.txt", std::ios_base::out);

    for (int i = 0; h * i <= 5; ++i)
    {
        auto current_state = p.getState();
        double x = p.getX();
        p.calculateNextIt();
        output << x << " " << current_state << std::endl;
    }
    output.close();
    return 0;
}