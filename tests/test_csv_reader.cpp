#include <gtest/gtest.h>
#include "nanomatch/io/csv_reader.hpp"
#include <fstream>
#include <cstdio>

using namespace nm;

class CSVReaderTest : public ::testing::Test {
protected:
    std::string tmp_path_;

    void SetUp() override {
        tmp_path_ = "/tmp/nanomatch_test_csv_reader_" +
                    std::to_string(::testing::UnitTest::GetInstance()
                                       ->current_test_info()
                                       ->line()) +
                    ".csv";
    }

    void TearDown() override {
        std::remove(tmp_path_.c_str());
    }

    void write_file(const std::string& content) {
        std::ofstream f(tmp_path_);
        f << content;
    }
};

TEST_F(CSVReaderTest, ReadValidCSVWithAllActionTypes) {
    write_file(
        "action,id,side,type,price,quantity\n"
        "NEW,1,BUY,LIMIT,10050,100\n"
        "CANCEL,2,SELL,LIMIT,9900,50\n"
        "MODIFY,3,BUY,LIMIT,10100,200\n");

    CSVReader reader(tmp_path_);
    auto orders = reader.read_orders();

    ASSERT_EQ(orders.size(), 3u);

    EXPECT_EQ(orders[0].action, OrderRequest::Action::New);
    EXPECT_EQ(orders[1].action, OrderRequest::Action::Cancel);
    EXPECT_EQ(orders[2].action, OrderRequest::Action::Modify);
}

TEST_F(CSVReaderTest, ParseAllOrderTypes) {
    write_file(
        "action,id,side,type,price,quantity\n"
        "NEW,1,BUY,LIMIT,10000,100\n"
        "NEW,2,BUY,MARKET,0,100\n"
        "NEW,3,BUY,IOC,10000,100\n"
        "NEW,4,BUY,FOK,10000,100\n");

    CSVReader reader(tmp_path_);
    auto orders = reader.read_orders();

    ASSERT_EQ(orders.size(), 4u);

    EXPECT_EQ(orders[0].type, OrderType::Limit);
    EXPECT_EQ(orders[1].type, OrderType::Market);
    EXPECT_EQ(orders[2].type, OrderType::ImmediateOrCancel);
    EXPECT_EQ(orders[3].type, OrderType::FillOrKill);
}

TEST_F(CSVReaderTest, ParseBothSides) {
    write_file(
        "action,id,side,type,price,quantity\n"
        "NEW,1,BUY,LIMIT,10000,100\n"
        "NEW,2,SELL,LIMIT,10100,200\n");

    CSVReader reader(tmp_path_);
    auto orders = reader.read_orders();

    ASSERT_EQ(orders.size(), 2u);

    EXPECT_EQ(orders[0].side, Side::Buy);
    EXPECT_EQ(orders[1].side, Side::Sell);
}

TEST_F(CSVReaderTest, ParseFieldValuesCorrectly) {
    write_file(
        "action,id,side,type,price,quantity\n"
        "NEW,42,SELL,LIMIT,9950,500\n");

    CSVReader reader(tmp_path_);
    auto orders = reader.read_orders();

    ASSERT_EQ(orders.size(), 1u);

    EXPECT_EQ(orders[0].id, 42u);
    EXPECT_EQ(orders[0].side, Side::Sell);
    EXPECT_EQ(orders[0].type, OrderType::Limit);
    EXPECT_EQ(orders[0].price, 9950);
    EXPECT_EQ(orders[0].quantity, 500u);
}

TEST_F(CSVReaderTest, EmptyFileReturnsNoOrders) {
    write_file("action,id,side,type,price,quantity\n");

    CSVReader reader(tmp_path_);
    auto orders = reader.read_orders();

    EXPECT_TRUE(orders.empty());
}

TEST_F(CSVReaderTest, FileNotFoundThrowsException) {
    CSVReader reader("/tmp/nanomatch_nonexistent_file_xyz.csv");
    EXPECT_THROW(reader.read_orders(), std::runtime_error);
}

TEST_F(CSVReaderTest, SkipsEmptyLines) {
    write_file(
        "action,id,side,type,price,quantity\n"
        "\n"
        "NEW,1,BUY,LIMIT,10000,100\n"
        "\n"
        "NEW,2,SELL,LIMIT,10100,200\n");

    CSVReader reader(tmp_path_);
    auto orders = reader.read_orders();

    EXPECT_EQ(orders.size(), 2u);
}
