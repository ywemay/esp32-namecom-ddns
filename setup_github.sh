#!/bin/bash
# Script to help set up GitHub repository for ESP32 Name.com DDNS project

set -e

echo "GitHub Repository Setup for ESP32 Name.com DDNS"
echo "================================================"

# Check if remote is already configured
if git remote -v | grep -q "origin"; then
    echo "Remote 'origin' is already configured:"
    git remote -v
else
    echo "No remote repository configured."
    echo ""
    echo "To set up GitHub:"
    echo "1. Go to https://github.com/new"
    echo "2. Create a new repository named 'esp32-namecom-ddns'"
    echo "3. DO NOT initialize with README, .gitignore, or license"
    echo "4. After creation, run these commands:"
    echo ""
    echo "   git remote add origin git@github.com:ywemay/esp32-namecom-ddns.git"
    echo "   git branch -M main"
    echo "   git push -u origin main"
    echo ""
fi

echo ""
echo "Current git status:"
git status

echo ""
echo "To push to GitHub (if remote is configured):"
echo "  git push origin main"