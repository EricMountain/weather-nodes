resource "aws_s3_bucket" "enry_weather_nodes_fmw_update" {
  bucket   = "enry-weather-nodes-fmw-update"
  provider = aws
  region   = "eu-north-1"
}

resource "aws_s3_bucket_lifecycle_configuration" "enry_weather_nodes_fmw_update_lifecycle" {
  bucket = aws_s3_bucket.enry_weather_nodes_fmw_update.id

  rule {
    id     = "Expire old files"
    status = "Enabled"

    filter {
      prefix = "" # Apply to all objects (empty prefix)
    }

    expiration {
      days = 1
    }

    noncurrent_version_expiration {
      noncurrent_days = 1
    }

    abort_incomplete_multipart_upload {
      days_after_initiation = 1
    }
  }
}

# AWS S3 Bucket Policy to allow Lambda function to generate presigned URLs for firmware updates

data "aws_lambda_function" "send_measurement_lambda" {
  function_name = "send-measurement"
  provider      = aws.paris
}

data "aws_iam_policy_document" "allow_lambda" {
  statement {
    principals {
      type        = "AWS"
      identifiers = [data.aws_lambda_function.send_measurement_lambda.role]
    }

    actions = [
      "s3:GetObject",
      "s3:ListBucket",
    ]

    effect = "Allow"

    resources = [
      aws_s3_bucket.enry_weather_nodes_fmw_update.arn,
      "${aws_s3_bucket.enry_weather_nodes_fmw_update.arn}/*",
    ]
  }
}

resource "aws_s3_bucket_policy" "enry_weather_nodes_fmw_update_policy" {
  bucket = aws_s3_bucket.enry_weather_nodes_fmw_update.id
  policy = data.aws_iam_policy_document.allow_lambda.json
}


# resource "aws_s3_bucket_policy" "enry_weather_nodes_fmw_update_policy" {
#   bucket   = aws_s3_bucket.enry_weather_nodes_fmw_update.id
#   policy   = data.aws_iam_policy_document.allow_lambda.json
# }

# data "aws_iam_policy_document" "allow_lambda" {
#   statement {
#     principals {
#       type        = "AWS"
#       identifiers = ["xxxx"]
#     }

#     actions = [
#       "s3:DeleteObject",
#       "s3:ListBucket",
#       "s3:PutObject",
#     ]

#     resources = [
#       aws_s3_bucket.enry_weather_nodes_fmw_update.arn,
#       "${aws_s3_bucket.enry_weather_nodes_fmw_update.arn}/*",
#     ]
#   }
# }
