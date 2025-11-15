from typing import Dict, Any

import logging
from datetime import datetime, timedelta, timezone
from zoneinfo import ZoneInfo

import boto3
from boto3.dynamodb.types import TypeDeserializer, TypeSerializer
from botocore.exceptions import ClientError

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
    is_valid, device_id, error_message, _ = authenticate_api_key(api_key)

    if not is_valid:
        if "API key missing" in error_message:
            return {"statusCode": 400, "body": error_message}
        elif "Invalid API key" in error_message:
            return {"statusCode": 401, "body": error_message}
        else:
            return {"statusCode": 500, "body": error_message}

    if method != "GET":
        return {"statusCode": 405, "body": "Method not allowed"}

    response = {
        "device_id": device_id,
    }

    # Get device config
    device_config_response = dynamodb.get_item(
        TableName="device_configs",
        Key={"device_id": {"S": device_id}},
    )
    if "Item" in device_config_response:
        device_config = dynamo_to_python(device_config_response["Item"])
    else:
        device_config = {}

    # Compute date and time
    if "location" in device_config and "local_timezone" in device_config["location"]:
        tz = ZoneInfo(device_config["location"]["local_timezone"])
    else:
        tz = ZoneInfo("Etc/UTC")

    local_now = datetime.now(tz)
    timestamp_local = local_now.isoformat(timespec="seconds")
    response["timestamp_local"] = timestamp_local

    now_utc = datetime.now(timezone.utc)
    timestamp_utc = now_utc.isoformat(timespec="seconds")
    response["timestamp_utc"] = timestamp_utc

    addLocationToResponse(response, device_config, local_now)

    # Get details for nodes this device should display
    if "nodes" in device_config:
        response["nodes"] = {}
        nodes = response["nodes"]
        for node in device_config["nodes"]:
            addNodeDataToResponse(nodes, node, now_utc)

    return {
        "statusCode": 200,
        "headers": {
            "Content-Type": "text/plain",
        },
        "body": str(response),
    }

def addNodeDataToResponse(nodes, node, now_utc):
    node_device_id = node["device_id"]
    node_display_name = node["display_name"]
    try:
        latest_measurements_response = dynamodb.get_item(
                    TableName="latest_measurements",
                    Key={"device_id": {"S": node_device_id}},
                )
    except ClientError as err:
        logger.error(
                    "Couldn't get measurement: %s: %s",
                    err.response["Error"]["Code"],
                    err.response["Error"]["Message"],
                )
        raise
    if "Item" in latest_measurements_response:
        latest_measurement = dynamo_to_python(
                    latest_measurements_response["Item"]
                )
    else:
        latest_measurement = {}

    nodes[node_device_id] = {}
    nodes[node_device_id]["display_name"] = node_display_name
    for m_version in ["measurements_v2", "status"]:
        if m_version in latest_measurement:
            nodes[node_device_id][m_version] = {}
            for k, v in latest_measurement[m_version].items():
                if m_version == "measurements_v2":
                    nodes[node_device_id][m_version][k] = {}
                    for sk, sv in v.items():
                        nodes[node_device_id][m_version][k][sk] = str(sv)
                else:
                    nodes[node_device_id][m_version][k] = str(v)

    addMinMaxToResponse(nodes, node_device_id, now_utc)

    if "timestamp_utc" in latest_measurement:
        nodes[node_device_id]["timestamp_utc"] = latest_measurement["timestamp_utc"]
    if "status" in latest_measurement:
        for k, v in latest_measurement["status"].items():
            nodes[node_device_id]["status"][k] = str(v)
    if "version" in latest_measurement:
        nodes[node_device_id]["version"] = str(latest_measurement["version"])

def addLocationToResponse(response, device_config, local_now):
    if "location" in device_config:
        response["config"] = {}
        response["config"]["location"] = {}
        response["config"]["location"]["utc_offset_seconds"] = int(local_now.utcoffset().total_seconds())
        for k, v in device_config["location"].items():
            response["config"]["location"][k] = str(v)

def addMinMaxToResponse(nodes, node_device_id, now_utc):
    now_minus_24h = (now_utc - timedelta(hours=24)).isoformat(timespec="seconds")
    try:
        measurements_today_response = dynamodb.query(
            TableName="measurements",
            KeyConditionExpression="device_id = :device_id AND timestamp_utc >= :start_timestamp_utc",
            ExpressionAttributeValues={
                ":device_id": {"S": node_device_id},
                ":start_timestamp_utc": {"S": now_minus_24h},
            },
        )
    except ClientError as err:
        logger.error(
            "Couldn't query measurements: %s: %s",
            err.response["Error"]["Code"],
            err.response["Error"]["Message"],
        )
        raise
    if "Items" in measurements_today_response:
        measurements_today = [
            dynamo_to_python(item) for item in measurements_today_response["Items"]
        ]
    else:
        measurements_today = []

    if len(measurements_today) == 0:
        return

    min_max = {}
    for measurement in measurements_today:
        if "measurements_v2" in measurement:
            for device, device_measurements in measurement["measurements_v2"].items():
                if device not in min_max:
                    min_max[device] = {}
                for measurement_name, measurement_value in device_measurements.items():
                    if measurement_name not in ["temperature", "humidity", "pressure"]:
                        continue
                    try:
                        value_as_float = float(measurement_value)
                    except (ValueError, TypeError):
                        continue
                    if measurement_name not in min_max[device]:
                        min_max[device][measurement_name] = {"min": value_as_float, "max": value_as_float}
                    else:
                        if value_as_float < min_max[device][measurement_name]["min"]:
                            min_max[device][measurement_name]["min"] = value_as_float
                        if value_as_float > min_max[device][measurement_name]["max"]:
                            min_max[device][measurement_name]["max"] = value_as_float

    nodes[node_device_id]["measurements_min_max"] = {}
    for device, device_measurements in min_max.items():
        if device_measurements is not None and len(device_measurements) > 0:
            nodes[node_device_id]["measurements_min_max"][device] = {}
        for measurement_name, measurement_value in device_measurements.items():
            nodes[node_device_id]["measurements_min_max"][device][measurement_name] = {
                "min": str(measurement_value["min"]),
                "max": str(measurement_value["max"]),
            }
