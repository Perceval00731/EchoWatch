# Explication du chiffrage du projet informatique

## 1. Données de base
- **Taux journalier encaissé (TJM)** : 300 € / jour (profils juniors)
- **Nombre de personnes mobilisées** : 4
- **Durée du projet** : 15 jours de travail effectif
- **RTT + congés payés** : 3,5 jours
- **Maintenance annuelle** : 3 jours
- **Serveur MQTT (VPS OVH)** : 50 € / an
- **Marge appliquée** : 10%
- **TVA** : 20%

---

## 2. Calcul du coût homme/jour
Pour une personne :

```math
300 \times (15 + 3,5 + 3) = 300 \times 21,5 = 6 450 \, €
```

Pour 4 personnes :

```math
6 450 \times 4 = 25 800 \, €
```

---

## 3. Coût infrastructure
- Serveur MQTT (VPS OVH) : **50 € / an**

### Justification du choix VPS OVH
- Hébergement en France/UE → conformité RGPD
- Contrôle total (accès root, possibilité d’ajouter TLS, ACL, supervision)
- Faible coût (≈ 50 €/an), plus économique que des solutions managées (AWS IoT, HiveMQ)
- Certaines solutions managées peuvent sembler moins chères au départ, mais leur modèle "pay as you go" fait rapidement augmenter le coût en cas de montée en charge ou d’utilisation intensive.

---

## 4. Total avant marge et TVA
```math
25 800 + 50 = 25 850 \, €
```

---

## 5. Ajout de la marge
Application d’une marge de **10%** :

```math
25 850 \times 1,10 = 28 435 \, € \, HT
```

---

## 6. TVA
TVA au taux de **20%** :

```math
28 435 \times 1,20 = 34 122 \, € \, TTC
```

---

## 7. Résumé
- **Coût main-d’œuvre** : 25 800 €
- **Infrastructure (serveur MQTT)** : 50 €
- **Sous-total** : 25 850 €
- **Marge (10%)** : 2 585 €
- **Total HT** : 28 435 €
- **TVA (20%)** : 5 687 €
- **Total TTC** : 34 122 €

---

> ✅ **Montant final facturé TTC : 34 122 €**
