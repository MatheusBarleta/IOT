package com.totem.api;

import org.springframework.data.jpa.repository.JpaRepository;

import java.util.List;

public interface LeituraRepository extends JpaRepository<Leitura, Long> {
    List<Leitura> findTop100ByOrderByIdDesc();
    long countByStatus(String status);
    long countByOffline(boolean offline);
}
