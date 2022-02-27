import React, { useEffect, useCallback, useState, useRef } from 'react'
import { Theme, createStyles, makeStyles } from '@material-ui/core/styles'
import CircularProgress from '@material-ui/core/CircularProgress'
import grey from '@material-ui/core/colors/grey'
import Grid from '@material-ui/core/Grid'
import Card from '@material-ui/core/Card'
import CardActionArea from '@material-ui/core/CardActionArea'
import CardActions from '@material-ui/core/CardActions'
import CardContent from '@material-ui/core/CardContent'
import Button from '@material-ui/core/Button'
import Typography from '@material-ui/core/Typography'
import TextField from '@material-ui/core/TextField'
import DeveloperBoardSharpIcon from '@material-ui/icons/DeveloperBoardSharp';

import Snackbar from '../snackbar'
import useLocalStorage from '../../hooks/useLocalStorage'
// import { initiateSocket, disconnectSocket, subscribeTo } from '../../utils/socket'

interface Device {
  pin: number
  label: string
  type: string
  status?: number
}

interface Props {
  devicesPath: string
}

interface DeviceListProps {
  devices: Device[] | null
}

interface DeviceItemProps {
  device: Device
}

interface MessageData {
  pin?: number
  status?: boolean
}

interface Message {
  eventName: string
  data: MessageData
}

const useStyles = makeStyles((theme: Theme) => createStyles({
  root: {
    flexGrow: 1,
  },
  card: {
    textAlign: 'center',
    color: theme.palette.text.secondary,
  },
  cardActions: {
    height: 50,
  },
  cardActionsRoot: {
    padding: 8,
    display: 'inline-flex',
  },
  media: {
    height: 140,
    width: 140,
    display: 'block',
    margin: 'auto',
  },
  actionItem: {
    display: 'inline-block',
  },
  deviceLabel: {
    color: grey[700],
  },
  parameter: {
    color: grey[500],
  },
}))

const DeviceItem = ({ device }: DeviceItemProps) => {
  const classes = useStyles()
  const [deviceStatus, setDeviceStatus] = useState(device.status)
  // const [serverBaseUrl] = useLocalStorage('serverBaseUrl')

  // force re-render
  if (deviceStatus !== device.status) {
    setDeviceStatus(device.status)
  }

  // const onButtonClick = async (statusToSet: number) => {
    // if (deviceStatus === statusToSet) {
    //   const res = await fetch(`${serverBaseUrl}/api/devices/change-status`, {
    //     method: 'POST',
    //     headers: {
    //       'Content-Type': 'application/json',
    //     },
    //     body: JSON.stringify({
    //       'pin': device.pin,
    //     }),
    //   })
    //   const { status } = await res.json()
    //   setDeviceStatus(status)
    // }
  // }

  const pFieldRef = useRef<HTMLInputElement | null>(null);
  const iFieldRef = useRef<HTMLInputElement | null>(null);
  const dFieldRef = useRef<HTMLInputElement | null>(null);

  const onPIDClick = async (p: string, i: string, d: string) => {
    const pid = {p: p, i: i, d: d}

    const res = await fetch('/pid', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(pid),
    })

    console.log(res)
    // setDevicePID([p, i, d])
  }


  let icon = null
  let iconStyles = { fontSize: 125 }
  let actions = null

  if (deviceStatus) {
    // @ts-ignore TS2322
    iconStyles = { ...iconStyles, fill: '#ffc300' }
  }
  icon = <DeveloperBoardSharpIcon style={iconStyles} />
  actions = (
    <>
      <Button size="small" color="primary" className={classes.actionItem} >
        {deviceStatus ? 'Disconnect' : 'Connect'}
      </Button>
    </>
  )
  

  return (
    <Grid item lg={3} md={4} xs={6}>
      <Card className={classes.card}>
        <CardActionArea>

            <>
              {icon}
              <CardContent>
                <Typography gutterBottom variant="h5" component="h2" className={classes.deviceLabel}>
                  {device.label}
                </Typography>

                <Typography gutterBottom variant="body1" component="span" className={classes.parameter}>
                  <TextField
                    id="standard-basic"
                    label="P"
                    variant="standard"
                    inputRef={pFieldRef}
                  />
                  <TextField
                    id="standard-basic"
                    label="I"
                    variant="standard"
                    inputRef={iFieldRef}
                  />
                  <TextField
                    id="standard-basic"
                    label="D"
                    variant="standard"
                    inputRef={dFieldRef}
                  />
                </Typography>
                <div>
                <Button size="small" color="primary" className={classes.actionItem} onClick={() => onPIDClick(pFieldRef.current?.value || "0", iFieldRef.current?.value || "0", dFieldRef.current?.value || "0")}>
                  Send
                </Button>
                </div>
              </CardContent>
            </>

        </CardActionArea>
        <CardActions className={classes.cardActions} classes={{ root: classes.cardActionsRoot }}>
          {actions}
        </CardActions>
      </Card>
    </Grid>
  )
}

const DeviceList = ({ devices }: DeviceListProps) => {
  const classes = useStyles()

  return (
    <div className={classes.root}>
      <Grid container spacing={3}>
        {devices && devices.map(device => <DeviceItem key={device.pin} device={device} />)}
      </Grid>
    </div>
  )
}

export default ({ devicesPath }: Props) => {
  const [serverBaseUrl] = useLocalStorage('serverBaseUrl')
  const [message, setMessage] = useState<Message | null>(null)
  const [data, setData] = useState<Device[] | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [loading, setLoading] = useState(false)

  const fetchData = useCallback(async () => {
    setMessage(null)
    setLoading(true)

    const data = [
      {
        pin: 2,
        type: 'led',
        label: 'Pi 1',
        status: 1,
        dependencies: [{ pin: 20, type: 'pushButton', label: 'Button 20' }],
      },
      {
        pin: 3,
        type: 'led',
        label: 'Pi 2',
        status: 0,
        dependencies: [{ pin: 20, type: 'pushButton', label: 'Button 20' }],
      },
      {
        pin: 4,
        type: 'led',
        label: 'Pi 3',
        status: 0,
        dependencies: [{ pin: 21, type: 'toggleButton', label: 'Button 21' }],
      },
      {
        pin: 20,
        type: 'pushButton',
        label: 'Pi 4',
        status: 0,
        dependencies: [{ pin: 2, type: 'led', label: 'Led 2' }, { pin: 3, type: 'led', label: 'Led 3' }],
      },
      {
        pin: 21,
        type: 'toggleButton',
        label: 'Pi 5',
        status: 0,
        dependencies: [{ pin: 4, type: 'led', label: 'Led 4' }],
      },
    ]

    // if (data.ok) {
    //   setData(await data.json())
    // } else {
    //   setError(await data.text())
    // }

    setData(data)

    setLoading(false)
  }, [])

  useEffect(() => {
    fetchData()
    fetch('/api').then(
      response => response.json()
    ).then(dataPID => console.log(dataPID))


  }, [fetchData, serverBaseUrl])

  if (message && data) {
    switch (message.eventName) {
    case 'deviceStatusChanged':
      const devices = data.map((device) => {
        if (device.pin === message.data.pin) {
          return {
            ...device,
            status: message.data.status,
          } as Device
        }
        return device
      }) as Device[]

      setData(devices)
      setMessage(null)
      break
    default:
      break
    }
  }

  return (
    <>
      <Snackbar message={error} severity="error" onClose={() => setError(null)}/>
      {!error ? loading ? <CircularProgress /> : <DeviceList devices={data} /> : null}
    </>
  )
}
