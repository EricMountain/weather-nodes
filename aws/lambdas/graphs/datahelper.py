"""
Data processing utilities for measurements.
"""
from typing import Dict, Any, List
from datetime import datetime, timezone
import logging
import boto3
from botocore.exceptions import ClientError
from dynamodb import dynamo_to_python

logger = logging.getLogger(__name__)
dynamodb = boto3.client("dynamodb")


def get_available_devices(device_id: str) -> List[Dict[str, str]]:
    """Get list of devices available for the given device_id"""
    try:
        # Get device config
        device_config_response = dynamodb.get_item(
            TableName="device_configs",
            Key={"device_id": {"S": device_id}},
        )
        
        devices = []
        if "Item" in device_config_response:
            device_config = dynamo_to_python(device_config_response["Item"])
            if "nodes" in device_config:
                for node in device_config["nodes"]:
                    devices.append({
                        "device_id": node["device_id"],
                        "display_name": node["display_name"]
                    })
        
        # If no devices found in config, at least include the main device
        if not devices:
            devices.append({
                "device_id": device_id,
                "display_name": "Main Device"
            })
        
        return devices
    
    except Exception as e:
        logger.error(f"Error getting available devices: {str(e)}")
        return [{"device_id": device_id, "display_name": "Main Device"}]


def get_measurements_data(device_ids: List[str], start_date: str, end_date: str, metric: str) -> Dict[str, Any]:
    """Get measurements data for the specified devices and date range"""
    try:
        # Parse dates
        start_datetime = datetime.fromisoformat(start_date).replace(tzinfo=timezone.utc)
        end_datetime = datetime.fromisoformat(end_date).replace(tzinfo=timezone.utc)
        
        all_data = {}
        
        for device_id in device_ids:
            try:
                measurements_response = dynamodb.query(
                    TableName="measurements",
                    KeyConditionExpression="device_id = :device_id AND timestamp_utc BETWEEN :start_time AND :end_time",
                    ExpressionAttributeValues={
                        ":device_id": {"S": device_id},
                        ":start_time": {"S": start_datetime.isoformat()},
                        ":end_time": {"S": end_datetime.isoformat()},
                    },
                )
                
                measurements = []
                if "Items" in measurements_response:
                    for item in measurements_response["Items"]:
                        measurement = dynamo_to_python(item)
                        measurements.append(measurement)
                
                # Process measurements to extract the specific metric
                processed_data = process_measurements_for_metric(measurements, metric)
                all_data[device_id] = processed_data
                
            except ClientError as err:
                logger.error(f"Error querying measurements for device {device_id}: {err}")
                all_data[device_id] = []
        
        return {
            "success": True,
            "metric": metric,
            "data": all_data,
            "start_date": start_date,
            "end_date": end_date
        }
    
    except Exception as e:
        logger.error(f"Error getting measurements data: {str(e)}")
        return {
            "success": False,
            "error": str(e)
        }


def process_measurements_for_metric(measurements: List[Dict], metric: str) -> List[Dict]:
    """Process measurements to extract data points for a specific metric"""
    data_points = []
    
    for measurement in measurements:
        timestamp = measurement.get("timestamp_utc", "")
        
        if "measurements_v2" in measurement:
            for device_name, device_measurements in measurement["measurements_v2"].items():
                if metric in device_measurements:
                    try:
                        value = float(device_measurements[metric])
                        data_points.append({
                            "timestamp": timestamp,
                            "value": value,
                            "device_name": device_name
                        })
                    except (ValueError, TypeError):
                        continue
    
    return sorted(data_points, key=lambda x: x["timestamp"])