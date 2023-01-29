#include <iostream>
#include <unistd.h>
#include <thread>

#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "bidichat-async-server.h"

using CPPUNIT_NS::TestCase;
using CPPUNIT_NS::TestResult;
using CPPUNIT_NS::TestResultCollector;
using CPPUNIT_NS::TestRunner;
using CPPUNIT_NS::TestFactoryRegistry;

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
    AsyncServer::ChatServer server;
    std::thread server_thread([&server]{server.Run();});
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

int main( int ac, char **av )
{
  TestResult controller;

  TestResultCollector result;
  controller.addListener( &result );        

  TestRunner runner;
  runner.addTest( TestFactoryRegistry::getRegistry().makeTest() );
  runner.run( controller );

  return result.wasSuccessful() ? 0 : 1;
}