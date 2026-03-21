#include <gtest/gtest.h>
#include "nanomatch/io/csv_writer.hpp"
#include <fstream>
#include <sstream>
#include <cstdio>

using namespace nm;

class CSVWriterTest : public ::testing::Test {
protected:
    std::string tmp_path_;

    void SetUp() override {
        tmp_path_ = "/tmp/nanomatch_test_csv_writer_" +
                    std::to_string(::testing::UnitTest::GetInstance()
                                       ->current_test_info()
                                       ->line()) +
                    ".csv";
    }

    void TearDown() override {
        std::remove(tmp_path_.c_str());
    }

    std::string read_file() {
        std::ifstream f(tmp_path_);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }
};

TEST_F(CSVWriterTest, WriteTradesProducesCorrectCSV) {
    std::vector<Trade> trades;
    trades.push_back(Trade{1, 2, 10050, 100, {}});
    trades.push_back(Trade{3, 4, 9900, 200, {}});

    CSVWriter writer(tmp_path_);
    writer.write_trades(trades);

    std::string content = read_file();
    std::istringstream ss(content);
    std::string line;

    // Verify header
    std::getline(ss, line);
    EXPECT_EQ(line, "buy_order_id,sell_order_id,price,quantity");

    // Verify first trade
    std::getline(ss, line);
    EXPECT_EQ(line, "1,2,10050,100");

    // Verify second trade
    std::getline(ss, line);
    EXPECT_EQ(line, "3,4,9900,200");
}

TEST_F(CSVWriterTest, HeaderIsCorrect) {
    std::vector<Trade> trades;
    trades.push_back(Trade{10, 20, 5000, 50, {}});

    CSVWriter writer(tmp_path_);
    writer.write_trades(trades);

    std::string content = read_file();
    EXPECT_EQ(content.substr(0, content.find('\n')),
              "buy_order_id,sell_order_id,price,quantity");
}

TEST_F(CSVWriterTest, AppendModeAddsWithoutHeader) {
    // First write with header
    CSVWriter writer(tmp_path_);
    writer.write_trades({Trade{1, 2, 10000, 100, {}}});

    // Append more trades
    writer.append_trades({Trade{3, 4, 10100, 200, {}}});

    std::string content = read_file();
    std::istringstream ss(content);
    std::string line;
    int line_count = 0;
    while (std::getline(ss, line)) {
        ++line_count;
    }

    // header + first trade + appended trade = 3 lines
    EXPECT_EQ(line_count, 3);
}

TEST_F(CSVWriterTest, EmptyTradeVectorWritesHeaderOnly) {
    CSVWriter writer(tmp_path_);
    writer.write_trades({});

    std::string content = read_file();
    EXPECT_EQ(content, "buy_order_id,sell_order_id,price,quantity\n");
}

TEST_F(CSVWriterTest, FieldValuesMatchTradeData) {
    Trade t{};
    t.buy_order_id = 42;
    t.sell_order_id = 99;
    t.price = 15075;
    t.quantity = 350;

    CSVWriter writer(tmp_path_);
    writer.write_trades({t});

    std::string content = read_file();
    // Check that the data line contains the correct values
    EXPECT_NE(content.find("42,99,15075,350"), std::string::npos);
}

TEST_F(CSVWriterTest, AppendPreservesExistingContent) {
    CSVWriter writer(tmp_path_);
    writer.write_trades({Trade{1, 2, 100, 10, {}}});
    writer.append_trades({Trade{5, 6, 200, 20, {}}});

    std::string content = read_file();
    EXPECT_NE(content.find("1,2,100,10"), std::string::npos);
    EXPECT_NE(content.find("5,6,200,20"), std::string::npos);
}
