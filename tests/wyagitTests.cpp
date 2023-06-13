#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <gtest/gtest.h>

#include "../CommandArguments.hpp"

std::filesystem::path REPO_PATH = std::filesystem::current_path() / "gitTest";

class GitCommandsTest : public ::testing::Test {
  public:

  protected:
    void SetUp() override
    {
        GitCommands::init(REPO_PATH);
    }

    void TearDown() override
    {
        cleanRoot();
    }

  private:
    void cleanRoot() { std::filesystem::remove_all(REPO_PATH); }
};

TEST_F(GitCommandsTest, GitInit)
{
    EXPECT_TRUE(std::filesystem::exists(REPO_PATH));
    EXPECT_TRUE(std::filesystem::exists(REPO_PATH / ".git"));

    auto pathToConfig = REPO_PATH / ".git" / "config";
    EXPECT_TRUE(std::filesystem::exists(pathToConfig));

    boost::property_tree::ptree config;
    boost::property_tree::read_ini(pathToConfig, config);
    ASSERT_EQ(config.get<int>("core.repositoryformatversion"), 0);
}

TEST_F(GitCommandsTest, HashFileBlob)
{
    auto textFile = REPO_PATH / "test.txt";
    Utilities::writeToFile(textFile, "text");
    auto fileHash = GitCommands::hashFile(textFile, "blob");
    auto fileHashFromFS = GitObject::findObject(fileHash.data(), "blob");
    EXPECT_EQ(fileHash.data(), fileHashFromFS.data());
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}