#!/usr/bin/env python3
"""
Script to get the record ID for a domain's A record from Name.com API.
This helps you find the RECORD_ID needed for the ESP32 project.

Usage:
1. Install requests: pip install requests
2. Update the credentials below
3. Run: python get_record_id.py
"""

import requests
import base64
import json

# Update these values
USERNAME = "your_username"
API_TOKEN = "your_api_token"
DOMAIN = "yourdomain.com"

def get_basic_auth(username, token):
    """Create Basic Auth header value"""
    credentials = f"{username}:{token}"
    encoded = base64.b64encode(credentials.encode()).decode()
    return f"Basic {encoded}"

def get_dns_records(domain):
    """Get all DNS records for a domain"""
    url = f"https://api.name.com/v4/domains/{domain}/records"
    headers = {
        "Authorization": get_basic_auth(USERNAME, API_TOKEN),
        "Content-Type": "application/json"
    }
    
    response = requests.get(url, headers=headers)
    
    if response.status_code == 200:
        return response.json()["records"]
    else:
        print(f"Error: {response.status_code}")
        print(response.text)
        return None

def main():
    print(f"Fetching DNS records for {DOMAIN}...")
    
    records = get_dns_records(DOMAIN)
    
    if records:
        print(f"\nFound {len(records)} records:")
        print("-" * 80)
        
        for record in records:
            if record["type"] == "A":
                print(f"ID: {record['id']}")
                print(f"Host: {record['host']}")
                print(f"Type: {record['type']}")
                print(f"Answer: {record['answer']}")
                print(f"TTL: {record['ttl']}")
                print(f"FQDN: {record['fqdn']}")
                print("-" * 80)
        
        print("\nFor the ESP32 project:")
        print("1. Find the A record you want to update dynamically")
        print("2. Copy the 'id' field as your RECORD_ID")
        print("3. Use the 'host' field as your RECORD_HOST")
        print("   - Use '@' for root domain")
        print("   - Use subdomain name (without domain) for subdomains")
    else:
        print("Failed to retrieve records")

if __name__ == "__main__":
    main()