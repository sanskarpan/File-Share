#include "PeerManager.h"
#include "Client.h"
#include <algorithm>

PeerManager::PeerManager() : running(false) {}

PeerManager::~PeerManager() {
    stop();
}

void PeerManager::start() {
    if (running.load()) return;
    
    running.store(true);
    heartbeat_thread = std::thread(&PeerManager::heartbeatLoop, this);
    
    // Connect to bootstrap nodes after a short delay
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        connectToBootstrapNodes();
    }).detach();
}

void PeerManager::stop() {
    if (!running.load()) return;
    
    running.store(false);
    
    if (heartbeat_thread.joinable()) {
        heartbeat_thread.join();
    }
}

void PeerManager::addPeer(std::shared_ptr<Peer> peer) {
    if (!peer) return;
    
    std::unique_lock<std::shared_mutex> lock(peers_mutex);
    peers[peer->getId()] = peer;
    
    std::cout << "Added peer: " << peer->getId() << " (" << peer->getAddress() << ")" << std::endl;
}

void PeerManager::removePeer(const std::string& peer_id) {
    std::unique_lock<std::shared_mutex> lock(peers_mutex);
    auto it = peers.find(peer_id);
    if (it != peers.end()) {
        std::cout << "Removed peer: " << peer_id << std::endl;
        peers.erase(it);
    }
}

std::shared_ptr<Peer> PeerManager::getPeer(const std::string& peer_id) const {
    std::shared_lock<std::shared_mutex> lock(peers_mutex);
    auto it = peers.find(peer_id);
    return (it != peers.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Peer>> PeerManager::getAllPeers() const {
    std::shared_lock<std::shared_mutex> lock(peers_mutex);
    std::vector<std::shared_ptr<Peer>> result;
    result.reserve(peers.size());
    
    for (const auto& pair : peers) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<std::shared_ptr<Peer>> PeerManager::getActivePeers() const {
    std::shared_lock<std::shared_mutex> lock(peers_mutex);
    std::vector<std::shared_ptr<Peer>> result;
    
    for (const auto& pair : peers) {
        if (pair.second->isActive()) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

void PeerManager::addBootstrapNode(const std::string& address, int port) {
    bootstrap_nodes.emplace_back(address, port);
}

void PeerManager::connectToBootstrapNodes() {
    for (const auto& [address, port] : bootstrap_nodes) {
        try {
            Client bootstrap_client;
            if (bootstrap_client.connect(address, port)) {
                // Request peer list from bootstrap node
                auto peer_list = bootstrap_client.requestPeerList();
                
                for (const auto& peer_data : peer_list) {
                    try {
                        auto peer = std::make_shared<Peer>(Peer::deserialize(peer_data));
                        addPeer(peer);
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to deserialize peer: " << e.what() << std::endl;
                    }
                }
                
                bootstrap_client.disconnect();
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to connect to bootstrap node " << address << ":" << port 
                      << " - " << e.what() << std::endl;
        }
    }
}

std::vector<std::shared_ptr<Peer>> PeerManager::findPeersWithFile(const std::string& filename) const {
    std::shared_lock<std::shared_mutex> lock(peers_mutex);
    std::vector<std::shared_ptr<Peer>> result;
    
    for (const auto& pair : peers) {
        if (pair.second->isActive() && pair.second->hasFile(filename)) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

void PeerManager::updatePeerFileList(const std::string& peer_id, const std::vector<FileInfo>& files) {
    std::shared_lock<std::shared_mutex> lock(peers_mutex);
    auto it = peers.find(peer_id);
    if (it != peers.end()) {
        // Clear existing files and add new ones
        auto peer = it->second;
        auto existing_files = peer->getFiles();
        
        // Remove all existing files
        for (const auto& file : existing_files) {
            peer->removeFile(file.filename);
        }
        
        // Add new files
        for (const auto& file : files) {
            peer->addFile(file);
        }
        
        peer->updateLastSeen();
    }
}

void PeerManager::heartbeatLoop() {
    while (running.load()) {
        removeStalePeers();
        broadcastPeerDiscovery();
        
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void PeerManager::removeStalePeers() {
    std::unique_lock<std::shared_mutex> lock(peers_mutex);
    auto now = std::chrono::system_clock::now();
    auto stale_threshold = std::chrono::minutes(5);
    
    auto it = peers.begin();
    while (it != peers.end()) {
        auto time_since_seen = now - it->second->getLastSeen();
        if (time_since_seen > stale_threshold) {
            std::cout << "Removing stale peer: " << it->first << std::endl;
            it = peers.erase(it);
        } else {
            ++it;
        }
    }
}

void PeerManager::broadcastPeerDiscovery() {
    auto active_peers = getActivePeers();
    
    for (auto& peer : active_peers) {
        try {
            Client client;
            if (client.connect(peer->getIpAddress(), peer->getPort())) {
                client.sendPing();
                client.disconnect();
                peer->updateLastSeen();
            } else {
                peer->setActive(false);
            }
        } catch (const std::exception& e) {
            peer->setActive(false);
        }
    }
}

size_t PeerManager::getActivePeerCount() const {
    std::shared_lock<std::shared_mutex> lock(peers_mutex);
    return std::count_if(peers.begin(), peers.end(),
        [](const auto& pair) { return pair.second->isActive(); });
}

size_t PeerManager::getTotalPeerCount() const {
    std::shared_lock<std::shared_mutex> lock(peers_mutex);
    return peers.size();
}