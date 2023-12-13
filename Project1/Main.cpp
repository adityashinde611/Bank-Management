#include <wx/wx.h>
#include <wx/grid.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <vector>
#include <iostream>
#include <sstream>

class Transaction {
public:
    std::string transactionType;
    double amount;
    std::string date;

    Transaction(const std::string& type, double amt, const std::string& dt)
        : transactionType(type), amount(amt), date(dt) {}
};

class BankAccount {
private:
    std::string accountNumber;
    std::string accountHolder;
    double balance;
    std::vector<Transaction> transactions;

public:
    BankAccount(std::string accNumber, std::string accHolder, double initialBalance)
        : accountNumber(accNumber), accountHolder(accHolder), balance(initialBalance) {}

    std::string getAccountNumber() const {
        return accountNumber;
    }

    std::string getAccountHolder() const {
        return accountHolder;
    }

    double getBalance() const {
        return balance;
    }

    const std::vector<Transaction>& getTransactionHistory() const {
        return transactions;
    }

    std::string GetCurrentDateAndTime() {
        wxDateTime now = wxDateTime::Now();
        return now.Format("%Y-%m-%d %H:%M:%S").ToStdString();
    }
    

    void deposit(double amount) {
        balance += amount;
        addTransaction("Deposit", amount, GetCurrentDateAndTime());
    }

    bool withdraw(double amount) {
        if (amount > balance) {
            return false; // Insufficient funds
        }
        else {
            balance -= amount;
            addTransaction("Withdrawal", amount, GetCurrentDateAndTime());
            return true;
        }
    }

    void addTransaction(const std::string& type, double amount, const std::string& date) {
        transactions.emplace_back(type, amount, date);
    }

};

class BankManagementFrame : public wxFrame {
private:
    wxGrid* grid;
    std::vector<BankAccount> accounts;
    sql::mysql::MySQL_Driver* driver;
    std::unique_ptr<sql::Connection> connection;

public:
    BankManagementFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size) {

        // Connect to MySQL database
        driver = sql::mysql::get_mysql_driver_instance();
        connection.reset(driver->connect("tcp://127.0.0.1:3306", "root", "Aditya")); 
        connection->setSchema("bank_management"); 

        // Create a grid to display accounts
        grid = new wxGrid(this, wxID_ANY);
        grid->CreateGrid(0, 3); // 3 columns for Account Number, Account Holder, and Balance

        // Set column labels
        grid->SetColLabelValue(0, "Account Number");
        grid->SetColLabelValue(1, "Account Holder");
        grid->SetColLabelValue(2, "Balance");


        grid->SetColSize(0, 200); // Set width for Account Number column
        grid->SetColSize(1, 200); // Set width for Account Holder column
        grid->SetColSize(2, 200);
        // Load accounts from the database
        LoadAccountsFromDatabase();

        // Create buttons
        wxButton* createButton = new wxButton(this, wxID_ANY, "Create Account");
        wxButton* depositButton = new wxButton(this, wxID_ANY, "Deposit");
        wxButton* withdrawButton = new wxButton(this, wxID_ANY, "Withdraw");
        wxButton* transferButton = new wxButton(this, wxID_ANY, "Transfer");
        wxButton* historyButton = new wxButton(this, wxID_ANY, "View History");
        wxButton* closeAccountButton = new wxButton(this, wxID_ANY, "Close Account");

        // Create horizontal box sizer for buttons
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        buttonSizer->Add(createButton, 0, wxALIGN_LEFT | wxALL, 5);
        buttonSizer->Add(depositButton, 0, wxALIGN_LEFT | wxALL, 5);
        buttonSizer->Add(withdrawButton, 0, wxALIGN_LEFT | wxALL, 5);
        buttonSizer->Add(transferButton, 0, wxALIGN_LEFT | wxALL, 5);
        buttonSizer->Add(historyButton, 0, wxALIGN_LEFT | wxALL, 5);
        buttonSizer->Add(closeAccountButton, 0, wxALIGN_LEFT | wxALL, 5);

        // Create vertical box sizer for main layout
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(grid, 1, wxEXPAND | wxALL, 5);
        mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);

        // Set the main sizer for the frame
        SetSizer(mainSizer);

        // Bind button events to functions
        Bind(wxEVT_BUTTON, &BankManagementFrame::OnCreateAccount, this, createButton->GetId());
        Bind(wxEVT_BUTTON, &BankManagementFrame::OnDeposit, this, depositButton->GetId());
        Bind(wxEVT_BUTTON, &BankManagementFrame::OnWithdraw, this, withdrawButton->GetId());
        Bind(wxEVT_BUTTON, &BankManagementFrame::OnTransfer, this, transferButton->GetId());
        Bind(wxEVT_BUTTON, &BankManagementFrame::OnViewHistory, this, historyButton->GetId());
        Bind(wxEVT_BUTTON, &BankManagementFrame::OnCloseAccount, this, closeAccountButton->GetId());
    }

    void LoadAccountsFromDatabase() {
        try {
            std::unique_ptr<sql::Statement> stmt(connection->createStatement());
            std::unique_ptr<sql::ResultSet> resultSet(stmt->executeQuery("SELECT * FROM accounts"));

            while (resultSet->next()) {
                std::string accNumber = resultSet->getString("account_number");
                std::string accHolder = resultSet->getString("account_holder");
                double balance = resultSet->getDouble("balance");

                accounts.emplace_back(accNumber, accHolder, balance);
            }

            UpdateGrid();
        }
        catch (const sql::SQLException& e) {
            wxMessageBox(wxString("SQL Error: ") + e.what(), "Error", wxOK | wxICON_ERROR);
        }
    }

    void UpdateGrid() {
        // Clear existing rows
        grid->DeleteRows(0, grid->GetNumberRows());

        // Add rows for each account
        for (size_t i = 0; i < accounts.size(); ++i) {
            grid->AppendRows();
            grid->SetCellValue(i, 0, accounts[i].getAccountNumber());
            grid->SetCellValue(i, 1, accounts[i].getAccountHolder());
            grid->SetCellValue(i, 2, wxString::Format("%.2f", accounts[i].getBalance()));
        }
    }

    void OnCreateAccount(wxCommandEvent& event) {
        wxString accNumber = wxGetTextFromUser("Enter Account Number:", "Create Account");
        wxString accHolder = wxGetTextFromUser("Enter Account Holder:", "Create Account");

        double initialBalance;
        wxString initialBalanceStr = wxGetTextFromUser("Enter Initial Balance:", "Create Account");
        initialBalanceStr.ToDouble(&initialBalance);

        BankAccount newAccount(accNumber.ToStdString(), accHolder.ToStdString(), initialBalance);
        accounts.push_back(newAccount);

        // Insert new account into the database
        InsertAccountIntoDatabase(newAccount);

        UpdateGrid();
    }

    void InsertAccountIntoDatabase(const BankAccount& account) {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                connection->prepareStatement("INSERT INTO accounts (account_number, account_holder, balance) VALUES (?, ?, ?)"));

            pstmt->setString(1, account.getAccountNumber());
            pstmt->setString(2, account.getAccountHolder());
            pstmt->setDouble(3, account.getBalance());

            pstmt->executeUpdate();
        }
        catch (const sql::SQLException& e) {
            wxMessageBox(wxString("SQL Error: ") + e.what(), "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnDeposit(wxCommandEvent& event) {
        wxString accNumber = wxGetTextFromUser("Enter Account Number:", "Deposit");

        size_t index = FindAccount(accNumber.ToStdString());
        if (index != wxNOT_FOUND) {
            double depositAmount;
            wxString depositAmountStr = wxGetTextFromUser("Enter Deposit Amount:", "Deposit");
            depositAmountStr.ToDouble(&depositAmount);

            accounts[index].deposit(depositAmount);

            // Update the database with the new balance
            UpdateAccountInDatabase(accounts[index]);

            UpdateGrid();
        }
        else {
            wxMessageBox("Account not found.", "Error", wxOK | wxICON_ERROR);
        }
    }

    void UpdateAccountInDatabase(const BankAccount& account) {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                connection->prepareStatement("UPDATE accounts SET balance = ? WHERE account_number = ?"));

            pstmt->setDouble(1, account.getBalance());
            pstmt->setString(2, account.getAccountNumber());

            pstmt->executeUpdate();
        }
        catch (const sql::SQLException& e) {
            wxMessageBox(wxString("SQL Error: ") + e.what(), "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnWithdraw(wxCommandEvent& event) {
        wxString accNumber = wxGetTextFromUser("Enter Account Number:", "Withdraw");

        size_t index = FindAccount(accNumber.ToStdString());
        if (index != wxNOT_FOUND) {
            double withdrawAmount;
            wxString withdrawAmountStr = wxGetTextFromUser("Enter Withdrawal Amount:", "Withdraw");
            withdrawAmountStr.ToDouble(&withdrawAmount);

            if (accounts[index].withdraw(withdrawAmount)) {
                // Update the database with the new balance
                UpdateAccountInDatabase(accounts[index]);

                UpdateGrid();
            }
            else {
                wxMessageBox("Insufficient funds. Withdrawal failed.", "Error", wxOK | wxICON_ERROR);
            }
        }
        else {
            wxMessageBox("Account not found.", "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnTransfer(wxCommandEvent& event) {
        wxString sourceAccNumber = wxGetTextFromUser("Enter Source Account Number:", "Transfer");
        wxString targetAccNumber = wxGetTextFromUser("Enter Target Account Number:", "Transfer");

        size_t sourceIndex = FindAccount(sourceAccNumber.ToStdString());
        size_t targetIndex = FindAccount(targetAccNumber.ToStdString());

        if (sourceIndex != wxNOT_FOUND && targetIndex != wxNOT_FOUND) {
            double transferAmount;
            wxString transferAmountStr = wxGetTextFromUser("Enter Transfer Amount:", "Transfer");
            transferAmountStr.ToDouble(&transferAmount);

            if (accounts[sourceIndex].withdraw(transferAmount)) {
                accounts[targetIndex].deposit(transferAmount);

                // Update both accounts in the database with the new balance
                UpdateAccountInDatabase(accounts[sourceIndex]);
                UpdateAccountInDatabase(accounts[targetIndex]);

                UpdateGrid();
            }
            else {
                wxMessageBox("Insufficient funds. Transfer failed.", "Error", wxOK | wxICON_ERROR);
            }
        }
        else {
            wxMessageBox("Source or target account not found.", "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnViewHistory(wxCommandEvent& event) {
        wxString accNumber = wxGetTextFromUser("Enter Account Number:", "View Transaction History");

        size_t index = FindAccount(accNumber.ToStdString());
        if (index != wxNOT_FOUND) {
            wxString history;
            const std::vector<Transaction>& transactions = accounts[index].getTransactionHistory();
            for (const Transaction& transaction : transactions) {
                history += wxString::Format("%s: %.2f on %s\n", transaction.transactionType, transaction.amount, transaction.date);
            }

            wxMessageBox(history, "Transaction History", wxOK | wxICON_INFORMATION);
        }
        else {
            wxMessageBox("Account not found.", "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnCloseAccount(wxCommandEvent& event) {
        wxString accNumber = wxGetTextFromUser("Enter Account Number to Close:", "Close Account");

        size_t index = FindAccount(accNumber.ToStdString());
        if (index != wxNOT_FOUND) {
            // Remove account from the database
            RemoveAccountFromDatabase(accounts[index]);

            // Remove account from the vector
            accounts.erase(accounts.begin() + index);

            UpdateGrid();
        }
        else {
            wxMessageBox("Account not found.", "Error", wxOK | wxICON_ERROR);
        }
    }

    void RemoveAccountFromDatabase(const BankAccount& account) {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                connection->prepareStatement("DELETE FROM accounts WHERE account_number = ?"));

            pstmt->setString(1, account.getAccountNumber());

            pstmt->executeUpdate();
        }
        catch (const sql::SQLException& e) {
            wxMessageBox(wxString("SQL Error: ") + e.what(), "Error", wxOK | wxICON_ERROR);
        }
    }

    size_t FindAccount(const std::string& accNumber) const {
        for (size_t i = 0; i < accounts.size(); ++i) {
            if (accounts[i].getAccountNumber() == accNumber) {
                return i; // Return the index of the account if found
            }
        }
        return wxNOT_FOUND; // Return wxNOT_FOUND if the account is not found
    }

    std::string GetCurrentDateAndTime() {
        wxDateTime now = wxDateTime::Now();
        return now.Format("%Y-%m-%d %H:%M:%S").ToStdString();
    }
};

class BankManagementApp : public wxApp {
public:
    virtual bool OnInit() {
        wxInitAllImageHandlers(); // Initialize image handlers for wxWidgets
        BankManagementFrame* frame = new BankManagementFrame("Bank Management System", wxPoint(50, 50), wxSize(800, 600));
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(BankManagementApp);
