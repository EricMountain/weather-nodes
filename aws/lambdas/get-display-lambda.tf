locals {
  get_display_lambda_zip = "tmp/get_display_lambda.zip"
}

data "archive_file" "get_display_lambda" {
  type        = "zip"
  source_dir  = "get-display"
  output_path = local.get_display_lambda_zip
}

resource "aws_lambda_function" "get_display_lambda" {
  filename         = local.get_display_lambda_zip
  function_name    = "get-display"
  handler          = "get-display.lambda_handler"
  source_code_hash = data.archive_file.get_display_lambda.output_base64sha256
  architectures    = ["arm64"]
  runtime          = "python3.13"
  role             = aws_iam_role.iam_for_get_display_lambda.arn
}

resource "aws_lambda_function_url" "get_display_lambda_url" {
  function_name      = aws_lambda_function.get_display_lambda.function_name
  authorization_type = "NONE"
}

output "get_display_lambda_url" {
  value = aws_lambda_function_url.get_display_lambda_url.function_url
}

resource "aws_iam_role" "iam_for_get_display_lambda" {
  name               = "iam_for_get_display_lambda"
  assume_role_policy = data.aws_iam_policy_document.lambda_assume_role.json
}

resource "aws_iam_policy" "get_display_lambda_policy" {
  name = "get_display_lambda_policy"

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
          "dynamodb:GetItem"
        ],
        Resource = aws_dynamodb_table.device_configs.arn
      },
      {
        Effect = "Allow",
        Action = [
          "dynamodb:GetItem"
        ],
        Resource = aws_dynamodb_table.latest_measurements.arn
      },
      {
        Effect = "Allow",
        Action = [
          "dynamodb:Query"
        ],
        Resource = aws_dynamodb_table.measurements.arn
      }
    ]
  })
}

resource "aws_iam_role_policy_attachment" "get_display_lambda_policy_attach" {
  role       = aws_iam_role.iam_for_get_display_lambda.name
  policy_arn = aws_iam_policy.get_display_lambda_policy.arn
}

# Uncomment for logging in CloudWatch
resource "aws_iam_role_policy_attachment" "get_display_lambda_attach_lambda_basic_execution" {
  role       = aws_iam_role.iam_for_get_display_lambda.name
  policy_arn = "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole"
}
