package com.totem.api;

import jakarta.validation.Valid;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;

import java.time.Instant;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api")
public class LeituraController {

    private final LeituraRepository repo;

    public LeituraController(LeituraRepository repo) {
        this.repo = repo;
    }

    /** Recebe o evento do totem (o ESP32 faz POST aqui). */
    @PostMapping("/leituras")
    @ResponseStatus(HttpStatus.CREATED)
    public LeituraResponse registrar(@Valid @RequestBody LeituraRequest req) {
        Leitura l = new Leitura();
        l.setTotemId(req.totemId());
        l.setUid(req.uid());
        l.setStatus(req.status());
        l.setOffline(req.offline());
        l.setUptimeMs(req.uptimeMs());
        l.setRecebidoEm(Instant.now());
        return LeituraResponse.de(repo.save(l));
    }

    /** Historico: as 100 leituras mais recentes. */
    @GetMapping("/leituras")
    public List<LeituraResponse> listar() {
        return repo.findTop100ByOrderByIdDesc().stream().map(LeituraResponse::de).toList();
    }

    /** Contadores para o painel. */
    @GetMapping("/resumo")
    public Map<String, Long> resumo() {
        Map<String, Long> r = new LinkedHashMap<>();
        r.put("total", repo.count());
        r.put("liberados", repo.countByStatus("LIBERADO"));
        r.put("negados", repo.countByStatus("NEGADO_DUPLICADO"));
        r.put("offline", repo.countByOffline(true));
        return r;
    }
}
