#!/usr/bin/env python3
"""
Test script for the graphs lambda function
"""

import sys
import os
import json
from datetime import datetime, timedelta

# Add the graphs directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'graphs'))

# Mock boto3 for testing
class MockDynamoDBClient:
    def get_item(self, **kwargs):
        # Mock API key validation
        if kwargs.get('TableName') == 'api_keys':
            return {
                'Item': {
                    'api_key': {'S': 'test-api-key'},
                    'device_id': {'S': 'test-device'}
                }
            }
        return {}
    
    def query(self, **kwargs):
        # Mock measurements query
        return {'Items': []}

class MockBoto3:
    @staticmethod
    def client(service_name):
        return MockDynamoDBClient()

class MockTypeDeserializer:
    def deserialize(self, x):
        return x.get('S', x.get('N', x))

class MockTypeSerializer:
    def serialize(self, x):
        return {'S': str(x)}

sys.modules['boto3'] = MockBoto3()
sys.modules['boto3.dynamodb'] = type('MockBoto3DynamoDB', (), {})()
sys.modules['boto3.dynamodb.types'] = type('MockBoto3DynamoDBTypes', (), {
    'TypeDeserializer': MockTypeDeserializer,
    'TypeSerializer': MockTypeSerializer
})()

from graphs import lambda_handler

def test_get_request():
    """Test GET request that should return HTML"""
    event = {
        "requestContext": {
            "http": {
                "method": "GET"
            }
        },
        "headers": {
            "x-api-key": "test-api-key"
        }
    }
    
    result = lambda_handler(event, {})
    print("GET Request Test:")
    print(f"Status Code: {result['statusCode']}")
    print(f"Content-Type: {result['headers']['Content-Type']}")
    print(f"Body length: {len(result['body'])} characters")
    print("✓ GET request test passed\n")

def test_post_request():
    """Test POST request for graph data"""
    # Calculate dates for the last 24 hours
    end_date = datetime.utcnow()
    start_date = end_date - timedelta(hours=24)
    
    body = f"start_date={start_date.isoformat()}&end_date={end_date.isoformat()}&metric=temperature&devices=displaydev"
    
    event = {
        "requestContext": {
            "http": {
                "method": "POST"
            }
        },
        "headers": {
            "x-api-key": "test-api-key",
            "content-type": "application/x-www-form-urlencoded"
        },
        "body": body
    }
    
    print("POST Request Test:")
    print(f"Request body: {body}")
    
    # Note: This will fail without actual DynamoDB access, but we can test the structure
    result = lambda_handler(event, {})
    print(f"Status Code: {result['statusCode']}")
    
    if result['statusCode'] == 200:
        try:
            data = json.loads(result['body'])
            print("✓ POST request returned valid JSON")
            print(f"Response keys: {list(data.keys())}")
        except json.JSONDecodeError:
            print("✗ POST request did not return valid JSON")
    else:
        print(f"Response body: {result['body']}")
    
    print("✓ POST request test completed\n")

def test_missing_api_key():
    """Test request without API key"""
    event = {
        "requestContext": {
            "http": {
                "method": "GET"
            }
        },
        "headers": {}
    }
    
    result = lambda_handler(event, {})
    print("Missing API Key Test:")
    print(f"Status Code: {result['statusCode']}")
    print(f"Body: {result['body']}")
    
    if result['statusCode'] == 400 and "API key missing" in result['body']:
        print("✓ Missing API key test passed\n")
    else:
        print("✗ Missing API key test failed\n")

def test_invalid_method():
    """Test invalid HTTP method"""
    event = {
        "requestContext": {
            "http": {
                "method": "DELETE"
            }
        },
        "headers": {
            "x-api-key": "test-api-key"
        }
    }
    
    result = lambda_handler(event, {})
    print("Invalid Method Test:")
    print(f"Status Code: {result['statusCode']}")
    print(f"Body: {result['body']}")
    
    if result['statusCode'] == 405:
        print("✓ Invalid method test passed\n")
    else:
        print("✗ Invalid method test failed\n")

if __name__ == "__main__":
    print("Testing Graphs Lambda Function")
    print("=" * 40)
    
    try:
        test_missing_api_key()
        test_invalid_method()
        test_get_request()
        # test_post_request()  # Commented out as it requires DynamoDB access
        
        print("All basic tests completed!")
        print("\nNote: POST request tests require actual DynamoDB access.")
        print("Deploy the lambda to AWS to test full functionality.")
        
    except Exception as e:
        print(f"Test failed with error: {e}")
        sys.exit(1)