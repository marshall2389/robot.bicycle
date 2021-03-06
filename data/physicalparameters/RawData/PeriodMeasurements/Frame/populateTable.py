import psycopg2
import scipy.io
import os

# Create a connection to RoboticBicycle database
conn = psycopg2.connect(database="robot_bicycle_parameters", user="hazelnusse")
cur = conn.cursor()

def insert_statement(cur, table, row):
    q = cur.mogrify("insert into " + table + " values(%s, %s, %s, %s, %s);", row)
    return q

angleOrder_dict = {'First':1, 'Second':2, 'Third':3, 'Fourth':4}

for subdir, dirs, files in os.walk('.'):
    i = 0
    j = 0
    files.sort()
    for file in files:
        if file.find('RobotFrameTorsional') != -1:
            matdata = scipy.io.loadmat(file)
            i += 1
            row = (i, int(angleOrder_dict[matdata['angleOrder'][0]]),
                   int(matdata['sampleRate'][0][0]),
                   int(matdata['duration'][0][0]),
                   matdata['data'].transpose()[0].tolist())
            SQL = insert_statement(
                    cur,
                    'parametermeasurements.frametorsionalpendulumtimeseries',
                    row)

            try:
                cur.execute(SQL)
                conn.commit()
            except (psycopg2.IntegrityError, psycopg2.InternalError) as inst:
                print("Exception in adding torsional pendulum data:")
                print(type(inst))
                print(inst)
                conn.rollback()
                continue
            print(cur.statusmessage)

        if file.find('RobotFrameCompound') != -1:
            matdata = scipy.io.loadmat(file)
            j += 1
            row = (j, 5,        # Compound pendulum is the 5th angle order
                   int(matdata['sampleRate'][0][0]),
                   int(matdata['duration'][0][0]),
                   matdata['data'].transpose()[0].tolist())
            SQL = insert_statement(
                    cur,
                    'parametermeasurements.framecompoundpendulumtimeseries',
                    row)

            try:
                cur.execute(SQL)
                conn.commit()
            except (psycopg2.IntegrityError, psycopg2.InternalError) as inst:
                print("Exception in adding compound pendulum data:")
                print(type(inst))
                print(inst)
                conn.rollback()
                continue
            print(cur.statusmessage)

cur.close()
conn.close()
