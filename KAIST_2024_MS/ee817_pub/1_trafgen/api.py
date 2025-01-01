#!/usr/bin/env python3
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, validator
from typing import Optional, Union, Tuple
import subprocess
from datetime import datetime
import asyncio
from apscheduler.schedulers.asyncio import AsyncIOScheduler
import os
from pathlib import Path
import ast
import ipaddress

from scapy.all import *
import ipaddress
import netifaces
import argparse
from typing import List, Union
from concurrent.futures import ThreadPoolExecutor
from threading import Lock
import math




def is_docker_network(ip):
    """Check if IP is in Docker's default network"""
    return ipaddress.ip_address(ip) in ipaddress.ip_network('172.17.0.0/24')

def get_interface_info(target_ip):
    """Get the appropriate interface based on target IP"""
    if is_docker_network(target_ip):
        iface = "eth0"
        try:
            addrs = netifaces.ifaddresses('eth0')
            ip = addrs[netifaces.AF_INET][0]['addr']
            mac = netifaces.ifaddresses('eth0')[netifaces.AF_LINK][0]['addr']
            print("Using docker0 interface for Docker network")
            return iface, ip, mac
        except KeyError:
            print("Docker interface not found!")
            sys.exit(1)
    else:
        # Use default interface (ens18) for non-Docker addresses
        gws = netifaces.gateways()
        default_iface = gws['default'][netifaces.AF_INET][1]
        addrs = netifaces.ifaddresses(default_iface)
        ip = addrs[netifaces.AF_INET][0]['addr']
        mac = netifaces.ifaddresses(default_iface)[netifaces.AF_LINK][0]['addr']
        print(f"Using {default_iface} interface for non-Docker network")
        return default_iface, ip, mac

def get_mac_from_arp(ip):
    """Get MAC address from ARP cache"""
    try:
        output = subprocess.check_output(['arp', '-n']).decode()
        for line in output.split('\n'):
            if ip in line:
                fields = line.split()
                if len(fields) >= 3:
                    return fields[2]
    except subprocess.CalledProcessError:
        pass
    return None

def resolve_mac(ip, iface):
    """Try ARP cache first, then active resolution if needed"""
    mac = get_mac_from_arp(ip)
    if mac:
        print(f"Found MAC in ARP cache: {mac}")
        return mac
        
    print(f"Not in ARP cache, trying active resolution...")
    ans, _ = srp(Ether(dst="ff:ff:ff:ff:ff:ff")/ARP(pdst=ip), timeout=2, iface=iface, verbose=False)
    if ans:
        return ans[0][1].hwsrc
    else:
        ans, _ = srp(Ether(dst="ff:ff:ff:ff:ff:ff")/ARP(pdst="172.17.0.1"), timeout=2, iface=iface, verbose=False)
        return ans[0][1].hwsrc

def generate_random_ip(start_ip, end_ip):
    """Generate a random IP between start and end IP"""
    start = int(ipaddress.IPv4Address(start_ip))
    end = int(ipaddress.IPv4Address(end_ip))
    random_ip = ipaddress.IPv4Address(random.randint(start, end))
    return str(random_ip)

def create_packet(src_mac, dst_mac, src_ip, dst_ip, dport, protocol, payload, seq_num=None):
    """Create a packet with proper Ethernet and TCP/UDP parameters"""
    if protocol == 'tcp':
        # Use much higher base numbers to make it look like mid-transmission
        base_seq = 1000000  # Start from a high number
        seq = base_seq + seq_num if seq_num is not None else random.randint(1000000, 2000000)
        ack_num = random.randint(1000000, 2000000)  # High ack number
        
        # PSH-ACK flags (0x18) - typical for data transmission
        flags = 'PA'  # Or use raw flags: flags=0x18
        window = 64240  # Common window size for established connections
        
        # Create the packet without checksums
        return Ether(src=src_mac, dst=dst_mac)/IP(src=src_ip, dst=dst_ip)/TCP(
            sport=random.randint(32768, 60999),
            dport=dport,
            seq=seq,
            ack=ack_num,
            flags=flags,
            window=window
        )/Raw(load=payload)
    else:
        return Ether(src=src_mac, dst=dst_mac)/IP(src=src_ip, dst=dst_ip)/UDP(
            sport=random.randint(32768, 60999),
            dport=dport
        )/Raw(load=payload)

def generate_packet_batch(args):
    """Generate a batch of packets with given parameters"""
    start_seq, count, src_mac, dst_mac, src_ip, dst_ip, port, protocol, payload = args
    batch_packets = []
    
    for i in range(count):
        seq_num = start_seq + i
        pkt = create_packet(src_mac, dst_mac, src_ip, dst_ip, port, protocol, payload, seq_num)
        batch_packets.append(pkt)
    
    return batch_packets

# Request/Response Models
class GenerateTrafficRequest(BaseModel):
    dst_ip: str  # Can be single IP or tuple string
    dst_port: Union[int, str]  # Can be single port or tuple string
    protocol: str
    packet_count: int = 1000000

    @validator('protocol')
    def validate_protocol(cls, v):
        if v.lower() not in ['tcp', 'udp']:
            raise ValueError('Protocol must be tcp or udp')
        return v.lower()

    @validator('dst_ip')
    def validate_ip(cls, v):
        if v.startswith('('):
            try:
                start_ip, end_ip = ast.literal_eval(v)
                ipaddress.ip_address(start_ip)
                ipaddress.ip_address(end_ip)
            except:
                raise ValueError('Invalid IP range format')
        else:
            try:
                ipaddress.ip_address(v)
            except:
                raise ValueError('Invalid IP address')
        return v

class SendTrafficRequest(BaseModel):
    pcap_file: str
    rate: str  # e.g., "10GiB"
    interface: str = "eth0"

class ScheduleSendRequest(BaseModel):
    pcap_file: str
    rate: str
    interface: str = "eth0"
    scheduled_time: datetime

app = FastAPI()

scheduler = AsyncIOScheduler(timezone=timezone.utc)


@app.on_event("startup")
async def startup_event():
    scheduler.start()
    print("Scheduler started!")  # Debug print


@app.on_event("shutdown")
async def shutdown_event():
    global scheduler
    if scheduler:
        scheduler.shutdown()

@app.post("/generate_traffic")
async def generate_traffic(request: GenerateTrafficRequest):
    try:
        # Determine target IP for interface selection
        target_ip = request.dst_ip if not request.dst_ip.startswith('(') else ast.literal_eval(request.dst_ip)[0]
        
        # Get interface information
        iface, src_ip, src_mac = get_interface_info(target_ip)
        
        # Resolve destination MAC address
        dst_mac = None
        if not request.dst_ip.startswith('('):
            dst_mac = resolve_mac(request.dst_ip, iface)
            if not dst_mac:
                raise HTTPException(status_code=400, detail=f"Could not find MAC address for {request.dst_ip}")

        # Process port
        if isinstance(request.dst_port, str) and request.dst_port.startswith('('):
            port_range = ast.literal_eval(request.dst_port)
            port = random.randint(int(port_range[0]), int(port_range[1]))
        else:
            port = int(request.dst_port)

        # Generate packets using threading
        num_threads = 8
        batch_size = math.ceil(request.packet_count / num_threads)
        payload = b'\xFF' * 1472

        batch_args = []
        for i in range(num_threads):
            start_seq = i * batch_size + 1
            count = min(batch_size, request.packet_count - (i * batch_size))
            if count <= 0:
                break
            batch_args.append((
                start_seq, count, src_mac, dst_mac, src_ip, request.dst_ip,
                port, request.protocol, payload
            ))

        all_packets = []
        with ThreadPoolExecutor(max_workers=num_threads) as executor:
            for batch in executor.map(generate_packet_batch, batch_args):
                all_packets.extend(batch)

        filename = f"generated_packets_{request.protocol}_{request.packet_count}.pcap"
        wrpcap(filename, all_packets)

        return {
            "status": "success",
            "packets_generated": len(all_packets),
            "filename": filename,
            "protocol": request.protocol
        }

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/send")
async def send_traffic(request: SendTrafficRequest):
    if not os.path.exists(request.pcap_file):
        raise HTTPException(status_code=404, detail="PCAP file not found")

    try:
        cmd = [
            "sudo", "tcpreplay",
            "--preload-pcap", "--topspeed",
            "-i", "eth0",
            request.pcap_file,
        ]
        
        process = await asyncio.create_subprocess_exec(
            *cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )
        
        stdout, stderr = await process.communicate()
        
        return {
            "status": "success" if process.returncode == 0 else "error",
            "stdout": stdout.decode(),
            "stderr": stderr.decode(),
            "return_code": process.returncode
        }

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/send_future")
async def schedule_send(request: ScheduleSendRequest):
    if not os.path.exists(request.pcap_file):
        raise HTTPException(status_code=404, detail="PCAP file not found")

    try:
        # Convert string to datetime if it isn't already
        if isinstance(request.scheduled_time, str):
            target_time = datetime.fromisoformat(request.scheduled_time)
        else:
            target_time = request.scheduled_time

        # Make both datetimes timezone-aware (UTC)
        now = datetime.now(timezone.utc).replace(second=0, microsecond=0)
        target_time = target_time.replace(tzinfo=timezone.utc, second=0, microsecond=0)
        
        job_id = f"traffic_job_{now.timestamp()}"

        print(f"Current time: {now}")  # Debug print
        print(f"Target time: {target_time}")  # Debug print

        if target_time <= now:
            print("Executing immediately")  # Debug print
            return await send_traffic(SendTrafficRequest(
                pcap_file=request.pcap_file,
                rate=request.rate,
                interface=request.interface
            ))

        print(f"Scheduling for future: {target_time}")  # Debug print
        scheduler.add_job(
            send_traffic,
            'date',
            run_date=target_time,
            args=[SendTrafficRequest(
                pcap_file=request.pcap_file,
                rate=request.rate,
                interface=request.interface
            )],
            id=job_id
        )

        return {
            "status": "scheduled",
            "job_id": job_id,
            "scheduled_time": target_time,
            "pcap_file": request.pcap_file
        }

    except Exception as e:
        print(f"Error: {str(e)}")  # Debug print
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
