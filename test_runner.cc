#include <iostream>
#include <unistd.h>
#include <thread>

#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
// #include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/CompilerOutputter.h>

#include "bidichat-async-server.h"
#include "bidichat-callback-client.h"

using CPPUNIT_NS::TestCase;
using CPPUNIT_NS::TestResult;
using CPPUNIT_NS::TestResultCollector;
using CPPUNIT_NS::TextUi::TestRunner;
using CPPUNIT_NS::TestFactoryRegistry;

class TestAsyncServer : public AsyncServer::ChatServer
{

};

class Test : public TestCase
{
  CPPUNIT_TEST_SUITE(Test);
  CPPUNIT_TEST(testAsyncServer);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {} 

protected:
  void testAsyncServer(void) {
    TestAsyncServer server;
    std::thread server_thread([&server]{server.Run();});

  CallbackClient::ChatClient chatclient( "localhost:50051", "test_client");

  // Send some junk
  chatclient.SendMessage("sup");
  chatclient.SendMessage("yo");
  chatclient.SendMessage("what is grpc?");

  server.Shutdown();
  server_thread.join();
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

int main( int ac, char **av )
{
  TestResult controller;

  TestResultCollector result;
  controller.addListener( &result );        

  CPPUNIT_NS::TestResultCollector collectedresults;
  controller.addListener(&collectedresults);

  // CPPUNIT_NS::BriefTestProgressListener progress;
  // controller.addListener( &progress );   

  TestRunner runner;
  runner.addTest( TestFactoryRegistry::getRegistry().makeTest() );
  runner.run( controller );

  CPPUNIT_NS::CompilerOutputter compileroutputter(&collectedresults, std::cerr);
  compileroutputter.write();

  return result.wasSuccessful() ? 0 : 1;
}