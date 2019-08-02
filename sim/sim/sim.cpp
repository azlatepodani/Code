// sim.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <stdio.h>
#include <vector>



struct book_entry_t {
    double assets = 0;
    double liabilities = 0;

    book_entry_t& operator+=(const book_entry_t& b);
};

//using book_t = std::vector<book_entry_t>;


struct Player {
    std::string name;
    book_entry_t balance;
};


std::vector<Player> g_players;

double avail_money = 0;
double outstanding_debt = 0;

book_entry_t operator+(const book_entry_t& a, const book_entry_t& b) {
    return book_entry_t{a.assets + b.assets, a.liabilities + b.liabilities};
}

book_entry_t& book_entry_t::operator+=(const book_entry_t& b) {
    *this = *this + b;
    return *this;
}


struct Credit {
    int iCreditor = 0;
    int iDebtor = 0;

    float amount = 0;
    float outstanding = 0;
    float interest = 0;
    float monthlyrate = 0;
    int intperiod = 0;
    int months = 0;
    int remaining = 0;
    bool credit_defaulted = 0;

    void award() {
        compute();

        book_entry_t payment{ amount, -outstanding };

        avail_money += amount;
        outstanding_debt += outstanding;

        g_players[iDebtor].balance += payment;
        payment.liabilities = -amount;
        g_players[iCreditor].balance += payment;
    }

    void compute() {
        auto im = interest / intperiod;
        auto tmp = pow(1 + im, months);
        auto tmp2 = (tmp - 1) / (im * tmp);
        monthlyrate = amount / tmp2;
        outstanding = monthlyrate * months;
    }

    void transfer(float val) {
        if (g_players[iDebtor].balance.assets < val) {
            credit_defaulted = true;
            remaining = 0;
            return;
        }

        book_entry_t payment{ -val, val };

        g_players[iDebtor].balance += payment;
        payment = book_entry_t{ -amount / months, amount / months };
        g_players[iCreditor].balance += payment;

        avail_money -= payment.liabilities;
        outstanding_debt -= val;

        payment = book_entry_t{ val - amount / months, 0 };
        g_players[iCreditor].balance += payment;
        outstanding -= val;
    }

    bool needs_service() { return remaining != 0; }

    bool operator()() {
        if (remaining > 1) {
            --remaining;
            transfer(monthlyrate);
        }
        else if (remaining == 1) {
            remaining = 0;
            transfer(outstanding);
        }
        else return false;

        return true;
    }
};

void print(const Credit& c) {
    auto& p1 = g_players[c.iCreditor];
    auto& p2 = g_players[c.iDebtor];

    printf("%s%8.3f\t%8.3f\t%8.3f\t%8.3f\t%8.3f\t%8.3f\n", c.credit_defaulted?"!":"",
                                                         p1.balance.assets, p1.balance.liabilities,
                                                         p2.balance.assets, p2.balance.liabilities,
                                                         avail_money, outstanding_debt);
}


Credit make_credit(
    int iCreditor,
    int iDebtor,
    float amount,
    float interest,
    int intperiod,
    int months)
{
    Credit c;
    c.iCreditor = iCreditor;
    c.iDebtor = iDebtor;
    c.amount = amount;
    c.interest = interest;
    c.intperiod = intperiod;
    c.months = months;
    c.remaining = months;
    return c;
}


int main()
{
    Player p1 = { "bank" };
    Player p2 = { "human" };

    g_players.push_back(p1); g_players.push_back(p2);

    Credit c = make_credit(0, 1, 10000.f, 0.05f, 12, 24);

    c.award();

    printf("Creditor\tDebtor\tMoney\tDebt\n");

    while (c()) print(c);

    //print(c);

    //std::cout << "Hello World!\n"; 
}
