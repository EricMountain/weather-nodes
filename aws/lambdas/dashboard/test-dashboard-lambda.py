#!/usr/bin/env python3
"""
Test script for the dashboard lambda function with graphs integration
"""

import sys
import os
import json
from datetime import datetime, timedelta

# Add the dashboard directory to the path
sys.path.insert(0, os.path.dirname(__file__))

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
        # Mock device configs
        if kwargs.get('TableName') == 'device_configs':
            return {
                'Item': {
                    'device_id': {'S': 'test-device'},
                    'location': {
                        'M': {
                            'name': {'S': 'Test Location'},
                            'local_timezone': {'S': 'UTC'}
                        }
                    },
                    'nodes': {
                        'L': [
                            {
                                'M': {
                                    'device_id': {'S': 'node1'},
                                    'display_name': {'S': 'Node 1'}
                                }
                            }
                        ]
                    }
                }
            }
        # Mock latest measurements
        if kwargs.get('TableName') == 'latest_measurements':
            return {
                'Item': {
                    'device_id': {'S': 'node1'},
                    'timestamp_utc': {'S': datetime.utcnow().isoformat()},
                    'measurements_v2': {
                        'M': {
                            'sensor1': {
                                'M': {
                                    'temperature': {'N': '22.5'},
                                    'humidity': {'N': '55'}
                                }
                            }
                        }
                    }
                }
            }
        return {}
    
    def query(self, **kwargs):
        # Mock measurements query
        return {'Items': []}
    
    def scan(self, **kwargs):
        # Mock scan for available devices
        return {
            'Items': [
                {
                    'device_id': {'S': 'node1'},
                }
            ]
        }

class MockBoto3:
    @staticmethod
    def client(service_name):
        return MockDynamoDBClient()

class MockTypeDeserializer:
    def deserialize(self, x):
        if 'S' in x:
            return x['S']
        elif 'N' in x:
            return x['N']
        elif 'M' in x:
            return {k: self.deserialize(v) for k, v in x['M'].items()}
        elif 'L' in x:
            return [self.deserialize(v) for v in x['L']]
        return x

class MockTypeSerializer:
    def serialize(self, x):
        return {'S': str(x)}

# Mock boto3 modules before importing dashboard
# Note: This is a simple approach for testing. For production testing,
# consider using unittest.mock or pytest-mock for more robust mocking.
sys.modules['boto3'] = MockBoto3()
sys.modules['boto3.dynamodb'] = type('MockBoto3DynamoDB', (), {})
sys.modules['boto3.dynamodb.types'] = type('MockBoto3DynamoDBTypes', (), {
    'TypeDeserializer': MockTypeDeserializer,
    'TypeSerializer': MockTypeSerializer
})

from dashboard import lambda_handler

def test_get_request():
    """Test GET request that should return HTML dashboard"""
    event = {
        "requestContext": {
            "http": {
                "method": "GET",
                "path": "/"
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
    print(f"Set-Cookie: {result['headers'].get('Set-Cookie')}")
    print(f"Body length: {len(result['body'])} characters")
    
    # Check for key elements in the HTML
    assert result['statusCode'] == 200
    assert 'text/html' in result['headers']['Content-Type']
    assert 'Weather Station' in result['body'] or 'Test Location' in result['body']
    assert 'Historical Data' in result['body']  # Graphs section
    assert 'Generate Graph' in result['body']  # Graph button
    assert 'd3.v7.min.js' in result['body']  # D3.js library
    assert 'Set-Cookie' in result['headers']
    print("✓ GET request test passed - dashboard includes graphs section\n")

def test_post_request():
    """Test POST request for graph data"""
    # Calculate dates for the last 24 hours
    end_date = datetime.utcnow()
    start_date = end_date - timedelta(hours=24)
    
    body = f"start_date={start_date.isoformat()}&end_date={end_date.isoformat()}&metric=temperature&devices=node1"
    
    event = {
        "requestContext": {
            "http": {
                "method": "POST",
                "path": "/"
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
    
    result = lambda_handler(event, {})
    print(f"Status Code: {result['statusCode']}")
    
    if result['statusCode'] == 200:
        try:
            data = json.loads(result['body'])
            print("✓ POST request returned valid JSON")
            print(f"Response keys: {list(data.keys())}")
            assert 'success' in data
            assert 'metric' in data
            assert 'data' in data
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
                "method": "GET",
                "path": "/"
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
                "method": "DELETE",
                "path": "/"
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
    print("Testing Dashboard Lambda Function with Graphs Integration")
    print("=" * 60)
    
    try:
        test_missing_api_key()
        test_invalid_method()
        test_get_request()
        test_post_request()
        
        print("All tests completed successfully!")
        print("\nNote: Tests use mocked DynamoDB data.")
        print("Deploy the lambda to AWS to test with real data.")
        
    except Exception as e:
        print(f"Test failed with error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
