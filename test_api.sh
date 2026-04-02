#!/bin/bash
# Test script to verify Name.com API access and update functionality

# Configuration - update these values
USERNAME="your_username"
API_TOKEN="your_api_token"
DOMAIN="yourdomain.com"
RECORD_ID="123456"  # Get this from the web interface or get_record_id.py script

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Name.com API Test Script${NC}"
echo "============================="

# Create Basic Auth header
AUTH_HEADER=$(echo -n "$USERNAME:$API_TOKEN" | base64)

echo -e "\n1. Testing API authentication..."
curl -s -X GET \
  -H "Authorization: Basic $AUTH_HEADER" \
  "https://api.name.com/v4/hello" | jq .

echo -e "\n2. Getting current DNS records for $DOMAIN..."
curl -s -X GET \
  -H "Authorization: Basic $AUTH_HEADER" \
  "https://api.name.com/v4/domains/$DOMAIN/records" | jq '.records[] | select(.type=="A")'

echo -e "\n3. Getting current IP address..."
CURRENT_IP=$(curl -s https://api.ipify.org)
echo "Current external IP: $CURRENT_IP"

echo -e "\n4. Testing DNS record update..."
# First, get the current record to preserve other fields
CURRENT_RECORD=$(curl -s -X GET \
  -H "Authorization: Basic $AUTH_HEADER" \
  "https://api.name.com/v4/domains/$DOMAIN/records/$RECORD_ID")

if [ $? -eq 0 ]; then
  echo "Current record:"
  echo "$CURRENT_RECORD" | jq .
  
  # Create update payload
  UPDATE_PAYLOAD=$(echo "$CURRENT_RECORD" | jq --arg ip "$CURRENT_IP" '.answer = $ip')
  
  echo -e "\nUpdate payload:"
  echo "$UPDATE_PAYLOAD" | jq .
  
  echo -e "\nSending update..."
  curl -s -X PUT \
    -H "Authorization: Basic $AUTH_HEADER" \
    -H "Content-Type: application/json" \
    -d "$UPDATE_PAYLOAD" \
    "https://api.name.com/v4/domains/$DOMAIN/records/$RECORD_ID" | jq .
    
  if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Update successful${NC}"
  else
    echo -e "${RED}✗ Update failed${NC}"
  fi
else
  echo -e "${RED}✗ Failed to get current record${NC}"
fi

echo -e "\n5. Verifying update..."
curl -s -X GET \
  -H "Authorization: Basic $AUTH_HEADER" \
  "https://api.name.com/v4/domains/$DOMAIN/records/$RECORD_ID" | jq '.answer'

echo -e "\n${YELLOW}Test complete.${NC}"
echo "If all steps passed, your API credentials are working correctly."