package com.totem.api;

import com.fasterxml.jackson.annotation.JsonProperty;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.PositiveOrZero;
import jakarta.validation.constraints.Size;

/** Payload enviado pelo ESP32. Toda entrada e validada antes de ir ao banco. */
public record LeituraRequest(
        @JsonProperty("totem_id") @NotBlank @Size(max = 50) String totemId,
        @NotBlank @Pattern(regexp = "^[0-9A-F]{4,20}$", message = "uid deve ser hexadecimal maiusculo") String uid,
        @NotBlank @Pattern(regexp = "^(LIBERADO|NEGADO_DUPLICADO)$", message = "status invalido") String status,
        @NotNull Boolean offline,
        @JsonProperty("uptime_ms") @PositiveOrZero Long uptimeMs
) {}
