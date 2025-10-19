from typing import Dict, Any

import base64
import json
import logging
from datetime import datetime, timedelta, timezone
from decimal import Decimal

import boto3
from boto3.dynamodb.types import TypeDeserializer, TypeSerializer
from botocore.exceptions import ClientError
from botocore.config import Config

from auth import extract_api_key, authenticate_api_key
from dynamodb import dynamo_to_python

logger = logging.getLogger(__name__)

dynamodb = boto3.client("dynamodb")
deserializer = TypeDeserializer()
serializer = TypeSerializer()


def lambda_handler(event: Dict[str, Any], context: Any) -> Dict[str, Any]:
    ctx = event.get("requestContext") or {}
    request = ctx.get("http") or {}
    method = request.get("method")

    api_key = extract_api_key(event)
    is_valid, device_id, error_message, api_key_response_item = authenticate_api_key(api_key)

    if not is_valid:
        if "API key missing" in error_message:
            return {"statusCode": 400, "body": error_message}
        elif "Invalid API key" in error_message:
            return {"statusCode": 401, "body": error_message}
        else:
            return {"statusCode": 500, "body": error_message}

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

    # Prepare the response
    response = {
        "device_id": device_id,
        "status": "ok",
    }

    check_for_ota_update(api_key_response_item, input, response)
    if "ota_update" in response:
        if "status" not in input:
            input["status"] = {}
        input["status"]["firmware_up_to_date"] = "no"

    # Prepare the measurements record to store
    timestamp_utc = datetime.now(timezone.utc).isoformat(timespec="seconds")
    item = {
        "device_id": serializer.serialize(device_id),
        "timestamp_utc": serializer.serialize(timestamp_utc),
    }

    for key in ["status", "measurements_v2", "version"]:
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



    return {
        "statusCode": 200,
        "headers": {
            "Content-Type": "text/plain",
        },
        "body": str(response),
    }

def check_for_ota_update(api_key_response_item, input, response):
    if "ota_update" in api_key_response_item and "version" in input:
        ota_update = api_key_response_item["ota_update"]
        if "target_version" in ota_update and input["version"] != ota_update["target_version"]:
            response["ota_update"] = {}
            s3 = boto3.client("s3", config=Config(signature_version="s3v4"), region_name="eu-north-1")
            try:
                presigned_url = s3.generate_presigned_url(
                    "get_object",
                    Params={
                        "Bucket": ota_update["s3_bucket"],
                        "Key": ota_update["s3_key"].format(target_version=ota_update["target_version"]),
                    },
                    ExpiresIn=3600, # seconds
                )
                response["ota_update"]["url"] = presigned_url
            except ClientError as e:
                logger.error(f"Error generating presigned URL: {e}")
                response["ota_update"] = {"error": "Could not generate download URL"}
