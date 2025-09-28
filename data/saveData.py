import asyncio
import websockets
import json
import logging
import datetime
from dotenv import load_dotenv
import os
from time import sleep

# Load .env file first
load_dotenv()

URL = "wss://stream.data.alpaca.markets/v1beta3/crypto/us"  # or "sip" for full feed

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(message)s',
    handlers=[
        logging.FileHandler('data/websocket.log'),
        logging.StreamHandler()
    ]
)

async def collect_data(websocket, output_file):
    try:
        with open(output_file, "w") as f:
            while True:
                msg = await websocket.recv()
                logging.info(f"Received: {msg}")
                f.write(msg + "\n")
    except asyncio.CancelledError:
        print("Data collection completed")

async def stream(runtime=60):
    try:
        logging.info(f"Connecting to {URL}")
        async with websockets.connect(URL) as ws:
            # Auth
            auth_response = await ws.recv()
            logging.info(f"Stat connections: {auth_response}")
            logging.info("Sending authentication")
            await asyncio.sleep(1)
            auth = json.dumps({
                "action": "auth",
                "key": os.getenv('API_KEY'),
                "secret": os.getenv('API_SECRET')
            })
            await ws.send(auth)
            auth_response = await ws.recv()
            logging.info(f"Auth Response: {auth_response}")
            
            await asyncio.sleep(1)
            
            # Subscribe
            logging.info("Subscribing to BTC/USD orderbook")
            await ws.send(json.dumps({
                "action": "subscribe",
                "orderbooks": ["BTC/USD"]
            }))
            auth_response = await ws.recv()
            logging.info(f"Auth Response: {auth_response}")
            await asyncio.sleep(1)
            
            # Create data collection task
            collection_task = asyncio.create_task(
                collect_data(ws, "data/alpaca_sample.json")
            )
            logging.info(f"Collecting data for {runtime} seconds...")
            # Wait for runtime seconds
            await asyncio.sleep(runtime)
            
            # Cancel collection task
            collection_task.cancel()
            try:
                await collection_task
            except asyncio.CancelledError:
                pass
    except Exception as e:
        logging.error(f"An error occurred: {e}")

if __name__ == "__main__":
    asyncio.run(stream(60))
