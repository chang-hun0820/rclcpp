// Copyright 2017 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <string>
#include <memory>
#include <vector>

#include "rclcpp/exceptions.hpp"
#include "rclcpp/rclcpp.hpp"

#include "rcl_interfaces/msg/intra_process_message.hpp"

class TestSubscription : public ::testing::Test
{
public:
  void OnMessage(const rcl_interfaces::msg::IntraProcessMessage::SharedPtr msg)
  {
    (void)msg;
  }

  static void SetUpTestCase()
  {
    rclcpp::init(0, nullptr);
  }

protected:
  void initialize(const rclcpp::NodeOptions & node_options = rclcpp::NodeOptions())
  {
    node = std::make_shared<rclcpp::Node>("test_subscription", "/ns", node_options);
  }

  void TearDown()
  {
    node.reset();
  }

  rclcpp::Node::SharedPtr node;
};

struct TestParameters
{
  TestParameters(rclcpp::QoS qos, std::string description)
  : qos(qos), description(description) {}
  rclcpp::QoS qos;
  std::string description;
};

std::ostream & operator<<(std::ostream & out, const TestParameters & params)
{
  out << params.description;
  return out;
}

class TestSubscriptionInvalidIntraprocessQos
  : public TestSubscription,
  public ::testing::WithParamInterface<TestParameters>
{};

class TestSubscriptionSub : public ::testing::Test
{
public:
  void OnMessage(const rcl_interfaces::msg::IntraProcessMessage::SharedPtr msg)
  {
    (void)msg;
  }

protected:
  static void SetUpTestCase()
  {
  }

  void SetUp()
  {
    node = std::make_shared<rclcpp::Node>("test_subscription", "/ns");
    subnode = node->create_sub_node("sub_ns");
  }

  void TearDown()
  {
    node.reset();
  }

  rclcpp::Node::SharedPtr node;
  rclcpp::Node::SharedPtr subnode;
};

class SubscriptionClassNodeInheritance : public rclcpp::Node
{
public:
  SubscriptionClassNodeInheritance()
  : Node("subscription_class_node_inheritance")
  {
  }

  void OnMessage(const rcl_interfaces::msg::IntraProcessMessage::SharedPtr msg)
  {
    (void)msg;
  }

  void CreateSubscription()
  {
    auto callback = std::bind(
      &SubscriptionClassNodeInheritance::OnMessage, this, std::placeholders::_1);
    using rcl_interfaces::msg::IntraProcessMessage;
    auto sub = this->create_subscription<IntraProcessMessage>("topic", 10, callback);
  }
};

class SubscriptionClass
{
public:
  void OnMessage(const rcl_interfaces::msg::IntraProcessMessage::SharedPtr msg)
  {
    (void)msg;
  }

  void CreateSubscription()
  {
    auto node = std::make_shared<rclcpp::Node>("test_subscription_member_callback", "/ns");
    auto callback = std::bind(&SubscriptionClass::OnMessage, this, std::placeholders::_1);
    using rcl_interfaces::msg::IntraProcessMessage;
    auto sub = node->create_subscription<IntraProcessMessage>("topic", 10, callback);
  }
};

/*
   Testing subscription construction and destruction.
 */
TEST_F(TestSubscription, construction_and_destruction) {
  initialize();
  using rcl_interfaces::msg::IntraProcessMessage;
  auto callback = [](const IntraProcessMessage::SharedPtr msg) {
      (void)msg;
    };
  {
    auto sub = node->create_subscription<IntraProcessMessage>("topic", 10, callback);
  }

  {
    ASSERT_THROW(
    {
      auto sub = node->create_subscription<IntraProcessMessage>("invalid_topic?", 10, callback);
    }, rclcpp::exceptions::InvalidTopicNameError);
  }
}

/*
   Testing subscription construction and destruction for subnodes.
 */
TEST_F(TestSubscriptionSub, construction_and_destruction) {
  using rcl_interfaces::msg::IntraProcessMessage;
  auto callback = [](const IntraProcessMessage::SharedPtr msg) {
      (void)msg;
    };
  {
    auto sub = subnode->create_subscription<IntraProcessMessage>("topic", 1, callback);
    EXPECT_STREQ(sub->get_topic_name(), "/ns/sub_ns/topic");
  }

  {
    auto sub = subnode->create_subscription<IntraProcessMessage>("/topic", 1, callback);
    EXPECT_STREQ(sub->get_topic_name(), "/topic");
  }

  {
    auto sub = subnode->create_subscription<IntraProcessMessage>("~/topic", 1, callback);
    std::string expected_topic_name =
      std::string(node->get_namespace()) + "/" + node->get_name() + "/topic";
    EXPECT_STREQ(sub->get_topic_name(), expected_topic_name.c_str());
  }

  {
    ASSERT_THROW(
    {
      auto sub = node->create_subscription<IntraProcessMessage>("invalid_topic?", 1, callback);
    }, rclcpp::exceptions::InvalidTopicNameError);
  }
}

/*
   Testing subscription creation signatures.
 */
TEST_F(TestSubscription, various_creation_signatures) {
  initialize();
  using rcl_interfaces::msg::IntraProcessMessage;
  auto cb = [](rcl_interfaces::msg::IntraProcessMessage::SharedPtr) {};
  {
    auto sub = node->create_subscription<IntraProcessMessage>("topic", 1, cb);
    (void)sub;
  }
  {
    auto sub = node->create_subscription<IntraProcessMessage>("topic", rclcpp::QoS(1), cb);
    (void)sub;
  }
  {
    auto sub =
      node->create_subscription<IntraProcessMessage>("topic", rclcpp::QoS(rclcpp::KeepLast(1)), cb);
    (void)sub;
  }
  {
    auto sub =
      node->create_subscription<IntraProcessMessage>("topic", rclcpp::QoS(rclcpp::KeepAll()), cb);
    (void)sub;
  }
  {
    auto sub = node->create_subscription<IntraProcessMessage>(
      "topic", 42, cb, rclcpp::SubscriptionOptions());
    (void)sub;
  }
  {
    auto sub = rclcpp::create_subscription<IntraProcessMessage>(
      node, "topic", 42, cb, rclcpp::SubscriptionOptions());
    (void)sub;
  }
}

/*
   Testing subscriptions using std::bind.
 */
TEST_F(TestSubscription, callback_bind) {
  initialize();
  using rcl_interfaces::msg::IntraProcessMessage;
  {
    // Member callback for plain class
    SubscriptionClass subscriptionObject;
    subscriptionObject.CreateSubscription();
  }
  {
    // Member callback for class inheriting from rclcpp::Node
    SubscriptionClassNodeInheritance subscriptionObject;
    subscriptionObject.CreateSubscription();
  }
  {
    // Member callback for class inheriting from testing::Test
    // Regression test for https://github.com/ros2/rclcpp/issues/479 where the TEST_F GTest macro
    // was interfering with rclcpp's `function_traits`.
    auto callback = std::bind(&TestSubscription::OnMessage, this, std::placeholders::_1);
    auto sub = node->create_subscription<IntraProcessMessage>("topic", 1, callback);
  }
}

/*
   Testing subscription with intraprocess enabled and invalid QoS
 */
TEST_P(TestSubscriptionInvalidIntraprocessQos, test_subscription_throws) {
  initialize(rclcpp::NodeOptions().use_intra_process_comms(true));
  rclcpp::QoS qos = GetParam().qos;
  using rcl_interfaces::msg::IntraProcessMessage;
  {
    auto callback = std::bind(
      &TestSubscriptionInvalidIntraprocessQos::OnMessage,
      this,
      std::placeholders::_1);

    ASSERT_THROW(
      {auto subscription = node->create_subscription<IntraProcessMessage>(
          "topic",
          qos,
          callback);},
      std::invalid_argument);
  }
}

static std::vector<TestParameters> invalid_qos_profiles()
{
  std::vector<TestParameters> parameters;

  parameters.reserve(3);
  parameters.push_back(
    TestParameters(
      rclcpp::QoS(rclcpp::KeepLast(10)).transient_local(),
      "transient_local_qos"));
  parameters.push_back(
    TestParameters(
      rclcpp::QoS(rclcpp::KeepAll()),
      "keep_all_qos"));

  return parameters;
}

INSTANTIATE_TEST_CASE_P(
  TestSubscriptionThrows, TestSubscriptionInvalidIntraprocessQos,
  ::testing::ValuesIn(invalid_qos_profiles()),
  ::testing::PrintToStringParamName());
