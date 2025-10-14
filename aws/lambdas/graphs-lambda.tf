locals {
  graphs_lambda_zip = "tmp/graphs_lambda.zip"
}

data "archive_file" "graphs_lambda" {
  type        = "zip"
  source_dir  = "graphs"
  output_path = local.graphs_lambda_zip
}

resource "aws_lambda_function" "graphs_lambda" {
  filename         = local.graphs_lambda_zip
  function_name    = "graphs"
  handler          = "graphs.lambda_handler"
  source_code_hash = data.archive_file.graphs_lambda.output_base64sha256
  architectures    = ["arm64"]
  runtime          = "python3.13"
  role             = aws_iam_role.iam_for_graphs_lambda.arn
  timeout          = 30

  environment {
    variables = {
      LOG_LEVEL = "INFO"
    }
  }

  tags = {
    Name = "Graphs Lambda"
    Environment = "prod"
  }
}

resource "aws_lambda_function_url" "graphs_lambda_url" {
  function_name      = aws_lambda_function.graphs_lambda.function_name
  authorization_type = "NONE"
}

output "graphs_lambda_url" {
  value = aws_lambda_function_url.graphs_lambda_url.function_url
}

resource "aws_iam_role" "iam_for_graphs_lambda" {
  name               = "iam_for_graphs_lambda"
  assume_role_policy = data.aws_iam_policy_document.lambda_assume_role.json
}

resource "aws_iam_policy" "graphs_lambda_policy" {
  name = "graphs_lambda_policy"

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
          "dynamodb:Query"
        ],
        Resource = aws_dynamodb_table.measurements.arn
      }
    ]
  })
}

resource "aws_iam_role_policy_attachment" "graphs_lambda_policy_attach" {
  role       = aws_iam_role.iam_for_graphs_lambda.name
  policy_arn = aws_iam_policy.graphs_lambda_policy.arn
}

# Uncomment for logging in CloudWatch
resource "aws_iam_role_policy_attachment" "graphs_lambda_attach_lambda_basic_execution" {
  role       = aws_iam_role.iam_for_graphs_lambda.name
  policy_arn = "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole"
}