locals {
  send_measurement_lambda_zip = "tmp/send_measurement_lambda.zip"
}

data "archive_file" "send_measurement_lambda" {
  type        = "zip"
  source_dir  = "send-measurement"
  output_path = local.send_measurement_lambda_zip
}

resource "aws_lambda_function" "send_measurement_lambda" {
  filename         = local.send_measurement_lambda_zip
  function_name    = "send-measurement"
  handler          = "send-measurement.lambda_handler"
  source_code_hash = data.archive_file.send_measurement_lambda.output_base64sha256
  architectures    = ["arm64"]
  runtime          = "python3.13"
  role             = aws_iam_role.iam_for_send_measurement_lambda.arn
}

resource "aws_lambda_function_url" "send_measurement_lambda_url" {
  function_name      = aws_lambda_function.send_measurement_lambda.function_name
  authorization_type = "NONE"
}

output "send_measurement_lambda_url" {
  value = aws_lambda_function_url.send_measurement_lambda_url.function_url
}

resource "aws_iam_role" "iam_for_send_measurement_lambda" {
  name               = "iam_for_send_measurement_lambda"
  assume_role_policy = data.aws_iam_policy_document.lambda_assume_role.json
}

resource "aws_iam_policy" "send_measurement_lambda_policy" {
  name = "send_measurement_lambda_policy"

  policy = jsonencode({
    Version = "2012-10-17",
    Statement = [
      {
        Effect = "Allow",
        Action = [
          "dynamodb:GetItem"
        ],
        Resource = aws_dynamodb_table.api_keys.arn
      },
      {
        Effect = "Allow",
        Action = [
          "dynamodb:PutItem"
        ],
        Resource = aws_dynamodb_table.latest_measurements.arn
      },
      {
        Effect = "Allow",
        Action = [
          "dynamodb:PutItem"
        ],
        Resource = aws_dynamodb_table.measurements.arn
      }
    ]
  })
}

resource "aws_iam_role_policy_attachment" "send_measurement_lambda_policy_attach" {
  role       = aws_iam_role.iam_for_send_measurement_lambda.name
  policy_arn = aws_iam_policy.send_measurement_lambda_policy.arn
}

# Uncomment for logging in CloudWatch
# resource "aws_iam_role_policy_attachment" "send_measurement_lambda_attach_lambda_basic_execution" {
#   role       = aws_iam_role.iam_for_send_measurement_lambda.name
#   policy_arn = "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole"
# }
