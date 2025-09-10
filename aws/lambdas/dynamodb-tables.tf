resource "aws_dynamodb_table" "api_keys" {
  name           = "api_keys"
  billing_mode   = "PAY_PER_REQUEST"
  hash_key       = "api_key"

  attribute {
    name = "api_key"
    type = "S"
  }

  tags = {
    Name = "API keys"
    Environment = "prod"
  }
}

resource "aws_dynamodb_table" "latest_measurements" {
  name           = "latest_measurements"
  billing_mode   = "PAY_PER_REQUEST"
  hash_key       = "device_id"

  attribute {
    name = "device_id"
    type = "S"
  }

  tags = {
    Name = "Latest measurements"
    Environment = "prod"
  }
}

resource "aws_dynamodb_table" "measurements" {
  name           = "measurements"
  billing_mode   = "PAY_PER_REQUEST"
  hash_key       = "device_id"
  range_key      = "timestamp_utc"

  attribute {
    name = "device_id"
    type = "S"
  }

  attribute {
    name = "timestamp_utc"
    type = "S"
  }

  ttl {
    attribute_name = "ttl"
    enabled        = true
  }

  tags = {
    Name = "Measurements"
    Environment = "prod"
  }
}

resource "aws_dynamodb_table" "device_configs" {
  name           = "device_configs"
  billing_mode   = "PAY_PER_REQUEST"
  hash_key       = "device_id"

  attribute {
    name = "device_id"
    type = "S"
  }

  tags = {
    Name = "Device configurations"
    Environment = "prod"
  }
}
