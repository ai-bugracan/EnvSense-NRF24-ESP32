import pandas as pd
from sklearn.preprocessing import LabelEncoder
import xgboost as xgb
import schedule
import time
import mysql.connector
from sklearn.model_selection import TimeSeriesSplit
from sklearn.metrics import accuracy_score

def label_environment(row):
    if row["mq135"] < 300:
        air = "temiz"
    else:
        air = "kirli"

    if row["ldr"] > 500:
        light = "gunduz"
    else:
        light = "gece"

    if row["temperature"] > 28:
        temp = "sicak"
    elif row["temperature"] < 18:
        temp = "soguk"
    else:
        temp = "normal"

    if row["humidity"] > 70:
        humidity = "nemli"
    elif row["humidity"] < 30:
        humidity = "kuru"
    else:
        humidity = "normal"

    return f"{air}-{light}-{temp}-{humidity}"

def run_prediction():
    print("Starting new prediction...")

    try:
        conn = mysql.connector.connect(
            host="your_host_name",
            user="your_user_name",
            password="your_password",
            database="your_database_name"
        )

        query = """
            SELECT time_col, temperature, humidity, mq135, ldr
            FROM sensor_data
            WHERE time_col >= NOW() - INTERVAL 70 MINUTE
            ORDER BY time_col ASC
        """
        df = pd.read_sql(query, conn)
        if df.empty or len(df) < 360:
            print("Not enough data in the last 60 minutes.")
            conn.close()
            return

        df = df.sort_values("time_col").reset_index(drop=True)

        df["label"] = df.apply(label_environment, axis=1)

        window_size = 60

        for i in range(window_size):
            df[f"temp_t-{i+1}"] = df["temperature"].shift(i+1)
            df[f"humidity_t-{i+1}"] = df["humidity"].shift(i+1)
            df[f"mq135_t-{i+1}"] = df["mq135"].shift(i+1)
            df[f"ldr_t-{i+1}"] = df["ldr"].shift(i+1)

        df["label_t+1"] = df["label"].shift(-1)

        feature_cols = []
        for i in range(window_size):
            feature_cols += [f"temp_t-{i+1}", f"humidity_t-{i+1}", f"mq135_t-{i+1}", f"ldr_t-{i+1}"]

        df_model = df.dropna(subset=feature_cols + ["label_t+1"])

        X = df_model[feature_cols]
        y = df_model["label_t+1"]

        le = LabelEncoder()
        y_encoded = le.fit_transform(y)

        # Time series cross-validation
        tscv = TimeSeriesSplit(n_splits=5)
        accuracies = []

        for train_index, test_index in tscv.split(X):
            X_train, X_test = X.iloc[train_index], X.iloc[test_index]
            y_train, y_test = y_encoded[train_index], y_encoded[test_index]

            model = xgb.XGBClassifier(
                objective='multi:softmax',
                num_class=len(le.classes_),
                eval_metric='mlogloss',
                use_label_encoder=False,
                random_state=42,
                n_estimators=300,
                max_depth=5,
                learning_rate=0.1
            )

            model.fit(X_train, y_train)
            preds = model.predict(X_test)
            acc = accuracy_score(y_test, preds)
            accuracies.append(acc)

        avg_acc = sum(accuracies) / len(accuracies)
        print(f"Time Series Cross-Validation Accuracy: %{avg_acc * 100:.2f}")

        # Son 60 veri ile tahmin
        last_rows = df.tail(window_size)
        sample = []
        for col in ["temperature", "humidity", "mq135", "ldr"]:
            vals = last_rows[col].values[::-1]
            sample.extend(vals)

        sample_df = pd.DataFrame([sample], columns=feature_cols)
        pred_encoded = model.predict(sample_df)[0]
        pred_label = le.inverse_transform([pred_encoded])[0]

        print("Predicted environment state 10 minutes ahead:", pred_label)

        cursor = conn.cursor()
        insert_sql = """
            INSERT INTO predictions (predicted_label, prediction_time)
            VALUES (%s, NOW())
        """
        cursor.execute(insert_sql, (pred_label,))
        conn.commit()
        cursor.close()
        conn.close()

        print("Prediction saved to database.")
        print("-" * 40)

    except Exception as e:
        print(f"An error occurred: {e}")

schedule.every(10).minutes.do(run_prediction)

print("Machine learning scheduler started. A prediction will be made every 10 minutes...")

while True:
    schedule.run_pending()
    time.sleep(1)
