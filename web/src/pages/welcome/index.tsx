import React from 'react'
import { Link } from 'react-router-dom'
import Button from '@material-ui/core/Button'
import Typography from '@material-ui/core/Typography'
import { Theme, makeStyles, createStyles } from '@material-ui/core/styles'

import Logo from '../../components/layout/logo'

const useStyles = makeStyles((theme: Theme) => createStyles({
  link: {
    textDecoration: 'none',
  },
  text: {
    textAlign: 'center',
    color: 'black',
  },
  disclamer: {
    marginTop: 50,
  },
}))

export default () => {
  const classes = useStyles()

  return (
    <>
      <Logo />

      <Typography variant="h3" className={classes.text} noWrap>
        Welcome!
      </Typography>

      <div className={classes.disclamer}>
        <Typography component="h6">
          In order to get started, you need to configure the app to connect it with your server
        </Typography>

        <Link to={'/settings'} className={classes.link}>
          <Button variant="outlined" color="primary">
            Go To Settings
          </Button>
        </Link>
      </div>
    </>
  )
}
