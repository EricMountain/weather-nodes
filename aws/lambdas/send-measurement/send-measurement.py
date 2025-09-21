from typing import Dict, Any

import base64
import json
import logging
from datetime import datetime, timedelta, timezone
from decimal import Decimal

import boto3
from boto3.dynamodb.types import TypeDeserializer, TypeSerializer
from botocore.exceptions import ClientError

logger = logging.getLogger(__name__)

dynamodb = boto3.client("dynamodb")
deserializer = TypeDeserializer()
serializer = TypeSerializer()


def lambda_handler(event: Dict[str, Any], context: Any) -> Dict[str, Any]:
    ctx = event.get("requestContext") or {}
    request = ctx.get("http") or {}
    method = request.get("method")

    device_id = None
    headers = event.get("headers") or {}
    api_key = headers.get("x-api-key") or headers.get("X-API-Key")
    if not api_key:
        return {"statusCode": 400, "body": "API key missing"}
    try:
        api_key_response = dynamodb.get_item(
            TableName="api_keys", Key={"api_key": {"S": api_key}}
        )
        if "Item" not in api_key_response:
            return {"statusCode": 401, "body": "Unauthorized: Invalid API key"}
        api_key_response_item = dynamo_to_python(api_key_response["Item"])
        if "device_id" not in api_key_response_item:
            return {"statusCode": 401, "body": "Unauthorized: Device ID not found"}
        device_id = api_key_response_item["device_id"]
    except Exception as e:
        return {"statusCode": 500, "body": f"Error checking API key: {str(e)}"}

    if method != "POST":
        return {
            "statusCode": 405,
            "message": "Method not allowed",
        }

    if "body" not in event:
        return {
            "statusCode": 400,
            "message": "Bad request",
        }
    if event["isBase64Encoded"]:
        body = base64.b64decode(event["body"]).decode("utf-8")
    else:
        body = event["body"]

    input = json.loads(body, parse_float=Decimal)

    timestamp_utc = datetime.now(timezone.utc).isoformat(timespec="seconds")
    item = {
        "device_id": serializer.serialize(device_id),
        "timestamp_utc": serializer.serialize(timestamp_utc),
    }

    for key in ["status", "measurements_v2"]:
        if key in input:
            item[key] = serializer.serialize(input[key])

    # First store the item in the latest measurements table
    try:
        dynamodb.put_item(TableName="latest_measurements", Item=item)
    except ClientError as err:
        logger.error(
            "Couldn't save latest measurement: %s: %s",
            err.response["Error"]["Code"],
            err.response["Error"]["Message"],
        )
        raise

    # Then store the item in the measurements table
    ttl = datetime.now(timezone.utc) + timedelta(days=90)
    item["ttl"] = serializer.serialize(int(ttl.timestamp()))

    try:
        dynamodb.put_item(TableName="measurements", Item=item)
    except ClientError as err:
        logger.error(
            "Couldn't save measurement: %s: %s",
            err.response["Error"]["Code"],
            err.response["Error"]["Message"],
        )
        raise

    response = {
        "device_id": device_id,
    }

    if "ota_update" in api_key_response_item and "version" in input:
        ota_update = api_key_response_item["ota_update"]
        if "target_version" in ota_update and input["version"] != ota_update["target_version"]:
            response["ota_update"] = ota_update

    return {
        "statusCode": 200,
        "headers": {
            "Content-Type": "text/plain",
        },
        "body": str(response),
    }


def dynamo_to_python(dynamo_object: dict) -> dict:
    deserializer = TypeDeserializer()
    return {k: deserializer.deserialize(v) for k, v in dynamo_object.items()}


def python_to_dynamo(python_object: dict) -> dict:
    serializer = TypeSerializer()
    return {k: serializer.serialize(v) for k, v in python_object.items()}
