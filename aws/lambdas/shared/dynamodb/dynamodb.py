"""
DynamoDB utility functions for the graphs lambda.
"""
from typing import Dict, Any
from boto3.dynamodb.types import TypeDeserializer, TypeSerializer

deserializer = TypeDeserializer()
serializer = TypeSerializer()


def dynamo_to_python(dynamo_object: dict) -> dict:
    """Convert DynamoDB item to Python dict"""
    return {k: deserializer.deserialize(v) for k, v in dynamo_object.items()}


def python_to_dynamo(python_object: dict) -> dict:
    """Convert Python dict to DynamoDB item"""
    return {k: serializer.serialize(v) for k, v in python_object.items()}