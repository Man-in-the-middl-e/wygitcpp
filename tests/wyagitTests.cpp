#include <gtest/gtest.h>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "../CommandArguments.hpp"

std::filesystem::path REPO_PATH = "gitTest";
class TestInitCleanup {
  public:
    TestInitCleanup() {}

    ~TestInitCleanup() { cleanRoot(); }

  private:
    void cleanRoot() { std::filesystem::remove_all(REPO_PATH); }
};

TEST(GitCommands, GitInit)
{
    GitCommands::init(REPO_PATH);
    EXPECT_TRUE(std::filesystem::exists(REPO_PATH));
    EXPECT_TRUE(std::filesystem::exists(REPO_PATH / ".git"));

    auto pathToConfig= REPO_PATH / ".git" / "config";
    EXPECT_TRUE(std::filesystem::exists(pathToConfig));

    boost::property_tree::ptree config;
    boost::property_tree::read_ini(pathToConfig, config);
    ASSERT_EQ(config.get<int>("core.repositoryformatversion"), 0);
}

int main(int argc, char* argv[])
{
    TestInitCleanup test;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}