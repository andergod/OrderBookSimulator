ENV_FILE=".env"
if [ -f "$ENV_FILE" ]; then
  echo "Loading environment variables from $ENV_FILE"
  # Export all non-comment, non-empty lines
  export $(grep -v '^#' "$ENV_FILE" | xargs)
else
  echo "No .env file found at $ENV_FILE, skipping environment load."
fi